
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#if !TEXT_USE_FREETYPE
# include "opengl/Font.hpp"
#endif
#include "opengl/Matrix.hpp"

#include "config.h"

#include "core/color.hpp"
#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"
#include "core/container/Stack.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"
#include "core/PngStructurer.hpp"
#include "core/serialization.hpp"
#include "core/sync/Event.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/resource/reader.hpp"

#include "utility/lookup_table.hpp"
#include "utility/ranges.hpp"
#include "utility/unicode.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>
#include <utility>

#if TEXT_USE_FREETYPE
# include <freetype2/ft2build.h>
# include FT_FREETYPE_H
# include FT_GLYPH_H
#endif

using namespace engine::graphics::opengl;

namespace engine
{
	namespace application
	{
		extern void make_current(window & window);
		extern void swap_buffers(window & window);
	}

	namespace graphics
	{
		namespace detail
		{
			extern core::async::Thread renderThread;
			extern std::atomic_int active;
			extern core::sync::Event<true> event;

			extern core::container::PageQueue<utility::heap_storage<DisplayMessage>> queue_displays;
			extern core::container::PageQueue<utility::heap_storage<AssetMessage>> queue_assets;
			extern core::container::PageQueue<utility::heap_storage<EntityMessage>> queue_entities;

			extern core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			extern std::atomic<int> entitytoggle;

			extern engine::graphics::renderer * self;
			extern engine::application::window * window;
			extern engine::resource::reader * reader;
			extern void (* callback_select)(engine::Entity entity, engine::Command command, utility::any && data);
		}
	}
}

debug_assets("box", "cuboid", "dude", "my_png", "photo");

namespace
{
	class Stack
	{
	public:
		using this_type = Stack;
		using value_type = core::maths::Matrix4x4f;

		static constexpr std::size_t capacity = 10;
	private:
		using prime_type = value_type::value_type;

	private:
		core::container::Stack<value_type, capacity> stack;

	public:
		Stack()
		{
			this->stack.emplace();
		}

	public:
		const value_type & top() const { return stack.top(); }

		void pop()
		{
			debug_assert(this->stack.size() > std::size_t{1});
			this->stack.pop();
		}
		void push()
		{
			debug_assert(this->stack.size() < capacity);
			this->stack.push(this->stack.top());
		}

		void load(const value_type matrix)
		{
			this->stack.top() = matrix;
		}
		void mult(const value_type matrix)
		{
			this->stack.top() *= matrix;
		}
		/**
		 * \note Use `mult` instead.
		 */
		void rotate(const core::maths::radian<prime_type> radian, const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::rotation(radian, x, y, z);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void scale(const prime_type xyz)
		{
			scale(xyz, xyz, xyz);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void scale(const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::scale(x, y, z);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void translate(const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::translation(x, y, z);
		}

	public:
		friend void glLoadMatrix(const Stack & stack)
		{
			glLoadMatrix(stack.stack.top());
		}
	};

	struct ShaderData
	{
		struct FragmentOutput
		{
			std::string name;
			int value;

			static constexpr auto serialization()
			{
				return make_lookup_table(
					std::make_pair(utility::string_view("name"), &FragmentOutput::name),
					std::make_pair(utility::string_view("value"), &FragmentOutput::value)
					);
			}
		};

		struct VertexInput
		{
			std::string name;
			int value;

			static constexpr auto serialization()
			{
				return make_lookup_table(
					std::make_pair(utility::string_view("name"), &VertexInput::name),
					std::make_pair(utility::string_view("value"), &VertexInput::value)
					);
			}
		};

		std::vector<VertexInput> inputs;
		std::vector<FragmentOutput> outputs;
		std::string vertex_source;
		std::string fragment_source;

		static constexpr auto serialization()
		{
			return make_lookup_table(
				std::make_pair(utility::string_view("inputs"), &ShaderData::inputs),
				std::make_pair(utility::string_view("outputs"), &ShaderData::outputs),
				std::make_pair(utility::string_view("vertex_source"), &ShaderData::vertex_source),
				std::make_pair(utility::string_view("fragment_source"), &ShaderData::fragment_source)
				);
		}
	};

	struct ShaderManager
	{
		engine::Asset assets[10];
		GLint programs[10];
		GLint vertices[10];
		GLint fragments[10];
		int count = 0;

		GLint get(engine::Asset asset) const
		{
			for (int i = 0; i < count; i++)
			{
				if (assets[i] == asset)
					return programs[i];
			}
			return -1;
		}

		GLint create(engine::Asset asset, ShaderData && shader_data)
		{
			GLint vs = glCreateShader(GL_VERTEX_SHADER);
			const char * vs_source = shader_data.vertex_source.c_str();
			glShaderSource(vs, 1, &vs_source, nullptr);
			glCompileShader(vs);
			GLint vs_compile_status;
			glGetShaderiv(vs, GL_COMPILE_STATUS, &vs_compile_status);
			if (!vs_compile_status)
			{
				char buffer[1000];
				int length;
				glGetShaderInfoLog(vs, 1000, &length, buffer);
				debug_printline("vertex shader entity failed to compile with: ", buffer);
			}

			GLint fs = glCreateShader(GL_FRAGMENT_SHADER);
			const char * fs_source = shader_data.fragment_source.c_str();
			glShaderSource(fs, 1, &fs_source, nullptr);
			glCompileShader(fs);
			GLint fs_compile_status;
			glGetShaderiv(fs, GL_COMPILE_STATUS, &fs_compile_status);
			if (!fs_compile_status)
			{
				char buffer[1000];
				int length;
				glGetShaderInfoLog(fs, 1000, &length, buffer);
				debug_printline("fragment shader entity failed to compile with: ", buffer);
			}

			GLint p = glCreateProgram();
			glAttachShader(p, vs);
			glAttachShader(p, fs);
			for (const auto & input : shader_data.inputs)
			{
				glBindAttribLocation(p, input.value, input.name.c_str());
			}
			for (const auto & output : shader_data.outputs)
			{
				glBindFragDataLocation(p, output.value, output.name.c_str());
			}
			glLinkProgram(p);
			GLint p_link_status;
			glGetProgramiv(p, GL_LINK_STATUS, &p_link_status);
			if (!p_link_status)
			{
				char buffer[1000];
				int length;
				glGetProgramInfoLog(p, 1000, &length, buffer);
				debug_printline("program entity failed to link with: ", buffer);
			}

			debug_assert(count < 10);
			assets[count] = asset;
			programs[count] = p;
			vertices[count] = vs;
			fragments[count] = fs;
			count++;

			debug_assert(glGetError() == GL_NO_ERROR);

			return p;
		}
	};

	void glUniform(GLint program, const char * const name, float v1)
	{
		const auto location = glGetUniformLocation(program, name);
		glUniform1f(location, v1);
	}
	void glUniform(GLint program, const char * const name, float v1, float v2)
	{
		const auto location = glGetUniformLocation(program, name);
		glUniform2f(location, v1, v2);
	}
	void glUniform(GLint program, const char * const name, int v1)
	{
		const auto location = glGetUniformLocation(program, name);
		glUniform1i(location, v1);
	}
	void glUniform(GLint program, const char * const name, const core::maths::Matrix4x4f & matrix)
	{
		const auto location = glGetUniformLocation(program, name);
		core::maths::Matrix4x4f::array_type buffer;
		glUniformMatrix4fv(location, 1, GL_FALSE, matrix.get(buffer));
	}
}

namespace
{
	constexpr std::array<GLenum, 10> BufferFormats =
	{{
		GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, (GLenum)-1, GL_BYTE, GL_SHORT, GL_INT, (GLenum)-1, GL_FLOAT, GL_DOUBLE
	}};

	struct display_t
	{
		int x;
		int y;
		int width;
		int height;

		core::maths::Matrix4x4f projection_3d;
		core::maths::Matrix4x4f frame_3d;
		core::maths::Matrix4x4f view_3d;
		core::maths::Matrix4x4f inv_projection_3d;
		core::maths::Matrix4x4f inv_frame_3d;
		core::maths::Matrix4x4f inv_view_3d;

		core::maths::Matrix4x4f projection_2d;
		core::maths::Matrix4x4f view_2d;

		core::maths::Vector2f from_world_to_frame(core::maths::Vector3f wpos) const
		{
			return to_xy(frame_3d * projection_3d * view_3d * to_xyz1(wpos));
		}
	};

	core::container::Collection
	<
		engine::Asset,
		21,
		utility::static_storage<10, display_t>
	>
	displays;

	struct update_display_camera_2d
	{
		engine::graphics::renderer::camera_2d && data;

		void operator () (display_t & x)
		{
			x.projection_2d = data.projection;
			x.view_2d = data.view;
		}
	};

	struct update_display_camera_3d
	{
		engine::graphics::renderer::camera_3d && data;

		void operator () (display_t & x)
		{
			x.projection_3d = data.projection;
			x.frame_3d = data.frame;
			x.view_3d = data.view;
			x.inv_projection_3d = data.inv_projection;
			x.inv_frame_3d = data.inv_frame;
			x.inv_view_3d = data.inv_view;
		}
	};

	struct update_display_viewport
	{
		engine::graphics::renderer::viewport && data;

		void operator () (display_t & x)
		{
			x.x = data.x;
			x.y = data.y;
			x.width = data.width;
			x.height = data.height;
		}
	};


	struct color_t
	{
		engine::graphics::opengl::Color4ub color;

		color_t(unsigned int color)
			: color((color & 0x000000ff) >>  0,
			        (color & 0x0000ff00) >>  8,
			        (color & 0x00ff0000) >> 16,
			        (color & 0xff000000) >> 24)
		{
		}

		void enable() const
		{
			glColor(color);
		}
		void disable() const
		{
		}
	};

	engine::graphics::opengl::Color4ub color_from(const engine::Entity entity)
	{
		return engine::graphics::opengl::Color4ub{
			(entity & 0x000000ff) >> 0,
			(entity & 0x0000ff00) >> 8,
			(entity & 0x00ff0000) >> 16,
			(entity & 0xff000000) >> 24 };
	}

	engine::graphics::opengl::Color4ub color_from(const engine::graphics::data::Color color)
	{
		return engine::graphics::opengl::Color4ub{
			(color & 0x000000ff) >> 0,
			(color & 0x0000ff00) >> 8,
			(color & 0x00ff0000) >> 16,
			(color & 0xff000000) >> 24 };
	}

	struct texture_t
	{
		GLuint id;

		int width;
		int height;

		~texture_t()
		{
			glDeleteTextures(1, &id);

			debug_assert(glGetError() == GL_NO_ERROR);
		}
		texture_t(core::graphics::Image && image)
		{
			glEnable(GL_TEXTURE_2D);

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			width = image.width();
			height = image.height();

			switch (image.color())
			{
			case core::graphics::ColorType::R:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, image.width(), image.height(), 0, GL_RED, BufferFormats[static_cast<int>(image.pixels().format())], image.data());
				break;
			case core::graphics::ColorType::RGB:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, BufferFormats[static_cast<int>(image.pixels().format())], image.data());
				break;
			case core::graphics::ColorType::RGBA:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, BufferFormats[static_cast<int>(image.pixels().format())], image.data());
				break;
			default:
				debug_unreachable();
			}

			debug_assert(glGetError() == GL_NO_ERROR);

			glDisable(GL_TEXTURE_2D);

			debug_assert(glGetError() == GL_NO_ERROR);
		}

		void enable() const
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			debug_assert(glGetError() == GL_NO_ERROR);
		}
		void disable() const
		{
			glDisable(GL_TEXTURE_2D);

			debug_assert(glGetError() == GL_NO_ERROR);
		}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		201,
		std::array<color_t, 100>,
		std::array<texture_t, 100>
	>
	materials;


	struct FontManager
	{
		struct SymbolData
		{
			int16_t offset_x;
			int16_t offset_y;
			int16_t advance_x;
			int16_t advance_y;
		};

		struct FontInfo
		{
			std::vector<utility::unicode_code_point> allowed_unicodes;
			std::vector<SymbolData> symbol_data;

			int symbol_width;
			int symbol_height;
			int texture_width; // ?
			int texture_height; // ?

			std::string name;
			int size;
		};

		engine::Asset assets[10];
		FontInfo infos[10];
		int count = 0;

		int find(engine::Asset asset) const
		{
			return std::find(assets, assets + count, asset) - assets;
		}

		bool has(engine::Asset asset) const
		{
			return find(asset) < count;
		}

		void compile(engine::Asset asset, utility::string_view_utf8 text, core::container::Buffer & vertices, core::container::Buffer & texcoords)
		{
			const int index = find(asset);
			debug_assert(index < count, "font asset ", asset, " does not exist");

			const FontInfo & info = infos[index];

			const int len = utility::point_difference(text.length()).get();
			vertices.resize<float>(4 * len * 2);
			texcoords.resize<float>(4 * len * 2);

			const int slots_in_width = info.texture_width / info.symbol_width;

			float * vertex_data = vertices.data_as<float>();
			float * texcoord_data = texcoords.data_as<float>();

			int x = 0;
			int y = 0;
			int i = 0;
			for (auto cp : text)
			{
				// the null glyph is stored at the end
				const auto maybe = std::lower_bound(info.allowed_unicodes.begin(), info.allowed_unicodes.end(), cp);
				const int slot = (maybe == info.allowed_unicodes.end() || *maybe != cp ? info.allowed_unicodes.end() : maybe) - info.allowed_unicodes.begin();
				const int slot_y = slot / slots_in_width;
				const int slot_x = slot % slots_in_width;

				const float blx = static_cast<float>(x + info.symbol_data[slot].offset_x);
				const float bly = static_cast<float>(y + info.symbol_data[slot].offset_y);
				const float trx = static_cast<float>(x + info.symbol_data[slot].offset_x + info.symbol_width);
				const float try_ = static_cast<float>(y + info.symbol_data[slot].offset_y - info.symbol_height);

				vertex_data[2 * 0 + 0] = blx;
				vertex_data[2 * 0 + 1] = bly;
				vertex_data[2 * 1 + 0] = trx;
				vertex_data[2 * 1 + 1] = bly;
				vertex_data[2 * 2 + 0] = trx;
				vertex_data[2 * 2 + 1] = try_;
				vertex_data[2 * 3 + 0] = blx;
				vertex_data[2 * 3 + 1] = try_;

				const float tex_blx = static_cast<float>(static_cast<double>(slot_x * info.symbol_width) / static_cast<double>(info.texture_width));
				const float tex_bly = static_cast<float>(static_cast<double>(slot_y * info.symbol_height) / static_cast<double>(info.texture_height));
				const float tex_trx = static_cast<float>(static_cast<double>((slot_x + 1) * info.symbol_width) / static_cast<double>(info.texture_width));
				const float tex_try = static_cast<float>(static_cast<double>((slot_y + 1) * info.symbol_height) / static_cast<double>(info.texture_height));

				texcoord_data[2 * 0 + 0] = tex_blx;
				texcoord_data[2 * 0 + 1] = tex_bly;
				texcoord_data[2 * 1 + 0] = tex_trx;
				texcoord_data[2 * 1 + 1] = tex_bly;
				texcoord_data[2 * 2 + 0] = tex_trx;
				texcoord_data[2 * 2 + 1] = tex_try;
				texcoord_data[2 * 3 + 0] = tex_blx;
				texcoord_data[2 * 3 + 1] = tex_try;

				vertex_data += 4 * 2;
				texcoord_data += 4 * 2;

				x += info.symbol_data[slot].advance_x;
				y += info.symbol_data[slot].advance_y;
				i++;
			}
		}

		void create(std::string && name, std::vector<utility::unicode_code_point> && allowed_unicodes, std::vector<SymbolData> && symbol_data, int symbol_width, int symbol_height, int texture_width, int texture_height)
		{
			const engine::Asset asset(name);
			const int index = find(asset);
			debug_assert(index >= count, "font asset ", asset, " already exists");

			assets[index] = asset;

			FontInfo & info = infos[index];
			info.allowed_unicodes = std::move(allowed_unicodes);
			info.symbol_data = std::move(symbol_data);
			info.symbol_width = symbol_width;
			info.symbol_height = symbol_height;
			info.texture_width = texture_width;
			info.texture_height = texture_height;
			info.name = std::move(name);
			info.size = 0; // ?

			count++;
		}

		void destroy(engine::Asset asset)
		{
			const int index = find(asset);
			debug_assert(index < count, "font asset ", asset, " does not exist");

			assets[index] = std::move(assets[count - 1]);
			infos[index] = std::move(infos[count - 1]);
			count--;
		}
	};


	struct mesh_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		core::container::Buffer coords;
		std::vector<engine::model::weight_t> weights;

		mesh_t(engine::graphics::data::Mesh && data)
			: modelview(core::maths::Matrix4x4f::identity())
			, vertices(std::move(data.vertices))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.coords))
		{}

		mesh_t(engine::model::mesh_t && data)
			: modelview(data.matrix)
			, vertices(std::move(data.xyz))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.uv))
			, weights(std::move(data.weights))
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		401,
		std::array<mesh_t, 100>,
		std::array<mesh_t, 1>
	>
	resources;


	struct object_modelview
	{
		core::maths::Matrix4x4f modelview;

		object_modelview(
			core::maths::Matrix4x4f && modelview)
			: modelview(std::move(modelview))
		{}
	};
	struct object_modelview_vertices
	{
		core::maths::Matrix4x4f modelview;

		core::container::Buffer vertices;

		object_modelview_vertices(
			const mesh_t & mesh,
			core::maths::Matrix4x4f && modelview)
			: modelview(std::move(modelview))
			, vertices(mesh.vertices)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Entity,
		1601,
		std::array<object_modelview, 600>,
		std::array<object_modelview_vertices, 200>
	>
	objects;

	struct update_modelview
	{
		engine::graphics::data::ModelviewMatrix && data;

		void operator () ()
		{
		}
		template <typename T>
		void operator () (T & x)
		{
			x.modelview = std::move(data.matrix);
		}
	};


	struct comp_c
	{
		struct asset
		{
			const mesh_t * mesh;
			engine::graphics::opengl::Color4ub color;

			asset(engine::graphics::data::CompC::asset && data)
				: mesh(&resources.get<mesh_t>(data.mesh))
				, color((data.color & 0x000000ff) >> 0,
						(data.color & 0x0000ff00) >> 8,
						(data.color & 0x00ff0000) >> 16,
						(data.color & 0xff000000) >> 24)
			{}
		};

		object_modelview * object;
		std::vector<asset> assets;

		comp_c(
			object_modelview & object,
			std::vector<engine::graphics::data::CompC::asset> && assets)
			: object(&object)
		{
			this->assets.reserve(assets.size());
			for (auto & asset : assets)
			{
				this->assets.emplace_back(std::move(asset));
			}
		}
	};

	struct comp_t
	{
		const mesh_t * mesh;
		const texture_t * texture;
		object_modelview * object;

		comp_t(
			const mesh_t & mesh,
			const texture_t & texture,
			object_modelview & object)
			: mesh(&mesh)
			, texture(&texture)
			, object(&object)
		{}
	};

	struct Character
	{
		const mesh_t * mesh;
		const texture_t * texture;
		object_modelview_vertices * object;

		Character(
			const mesh_t & mesh,
			const texture_t & texture,
			object_modelview_vertices & object)
			: mesh(&mesh)
			, texture(&texture)
			, object(&object)
		{}
	};

	struct Bar
	{
		core::maths::Vector3f worldPosition;
		float progress;

		Bar(engine::graphics::data::Bar && bar)
			: worldPosition(bar.worldPosition)
			, progress(bar.progress)
		{}
	};

	struct linec_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer edges;
		engine::graphics::opengl::Color4ub color;

		engine::graphics::opengl::Color4ub selectable_color = {0, 0, 0, 0};

		linec_t(engine::graphics::data::LineC && data) :
			modelview(std::move(data.modelview)),
			vertices(std::move(data.vertices)),
			edges(std::move(data.edges)),
			color((data.color & 0x000000ff) >>  0,
			      (data.color & 0x0000ff00) >>  8,
			      (data.color & 0x00ff0000) >> 16,
			      (data.color & 0xff000000) >> 24)
		{}
		linec_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	namespace ui
	{
		struct PanelC
		{
			object_modelview * object;
			core::maths::Vector2f size;
			engine::graphics::opengl::Color4ub color;

			PanelC(
				object_modelview & object,
				core::maths::Vector2f && size,
				unsigned int && color)
				: object(&object)
				, size(std::move(size))
				, color(color_from(std::move(color)))
			{}
		};

		struct PanelT
		{
			const texture_t * texture;
			object_modelview * object;
			core::maths::Vector2f size;

			PanelT(
				const texture_t & texture,
				object_modelview & object,
				core::maths::Vector2f && size)
				: texture(&texture)
				, object(&object)
				, size(std::move(size))
			{}
		};

		struct Text
		{
			object_modelview * object;
			std::string display;
			engine::graphics::opengl::Color4ub color;

			Text(
				object_modelview & object,
				std::string && display,
				unsigned int && color)
				: object(&object)
				, display(std::move(display))
				, color(color_from(color))
			{}
		};
	}

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<comp_c>,
		utility::heap_storage<comp_t>,
		utility::heap_storage<Character>,
		utility::heap_storage<Bar>,
		utility::heap_storage<linec_t>,
		utility::heap_storage<::ui::PanelC>,
		utility::heap_storage<::ui::PanelT>,
		utility::heap_storage<::ui::Text>
	>
	components;


	struct selectable_character_t
	{
		const mesh_t * mesh;
		object_modelview_vertices * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_character_t(
			const mesh_t * mesh,
			object_modelview_vertices * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: mesh(mesh)
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_comp_c
	{
		std::vector<const mesh_t *> meshes;
		object_modelview * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_comp_c(
			std::vector<const mesh_t *> meshes,
			object_modelview * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: meshes(std::move(meshes))
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_comp_t
	{
		const mesh_t * mesh;
		object_modelview * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_comp_t(
			const mesh_t * mesh,
			object_modelview * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: mesh(mesh)
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_panel
	{
		object_modelview * object;
		core::maths::Vector2f size;
		engine::graphics::opengl::Color4ub selectable_color;

		selectable_panel(
			object_modelview * object,
			core::maths::Vector2f size,
			engine::graphics::opengl::Color4ub selectable_color)
			: object(object)
			, size(std::move(size))
			, selectable_color(std::move(selectable_color))
		{}
	};

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<selectable_character_t>,
		utility::heap_storage<selectable_comp_c>,
		utility::heap_storage<selectable_comp_t>,
		utility::heap_storage<selectable_panel>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		void operator () (engine::Entity entity, comp_c & x)
		{
			std::vector<const mesh_t *> meshes;
			meshes.reserve(x.assets.size());
			for (const auto & asset : x.assets)
			{
				meshes.push_back(asset.mesh);
			}
			debug_verify(selectable_components.try_emplace<selectable_comp_c>(entity, meshes, x.object, color));
		}
		void operator () (engine::Entity entity, comp_t & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_comp_t>(entity, x.mesh, x.object, color));
		}
		void operator () (engine::Entity entity, Character & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_character_t>(entity, x.mesh, x.object, color));
		}
		void operator () (engine::Entity entity, ui::PanelC & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_panel>(entity, x.object, x.size, color));
		}
		void operator () (engine::Entity entity, ui::PanelT & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_panel>(entity, x.object, x.size, color));
		}

		template <typename T>
		void operator () (engine::Entity entity, T & x)
		{
			debug_unreachable();
		}
	};

	struct set_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		template <typename T>
		void operator () (T & x)
		{
			x.selectable_color = color;
		}
	};


	struct updateable_character_t
	{
		const mesh_t * mesh;
		object_modelview_vertices * object;

		std::vector<core::maths::Matrix4x4f> matrix_pallet;

		updateable_character_t(
			const mesh_t & mesh,
			object_modelview_vertices & object)
			: mesh(&mesh)
			, object(&object)
		{}

		void update()
		{
			if (matrix_pallet.empty())
				return;

			const float * const untransformed_vertices = mesh->vertices.data_as<float>();
			float * const transformed_vertices = object->vertices.data_as<float>();
			const int nvertices = object->vertices.count() / 3; // assume xyz
			for (int i = 0; i < nvertices; i++)
			{
				const core::maths::Vector4f vertex = matrix_pallet[mesh->weights[i].index] * core::maths::Vector4f{untransformed_vertices[3 * i + 0], untransformed_vertices[3 * i + 1], untransformed_vertices[3 * i + 2], 1.f};
				core::maths::Vector4f::array_type buffer;
				vertex.get(buffer);
				transformed_vertices[3 * i + 0] = buffer[0];
				transformed_vertices[3 * i + 1] = buffer[1];
				transformed_vertices[3 * i + 2] = buffer[2];
				// assume buffer[3] == 1.f
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<updateable_character_t>
	>
	updateable_components;

	struct update_matrixpallet
	{
		engine::graphics::renderer::CharacterSkinning && data;

		void operator () ()
		{
		}
		void operator () (updateable_character_t & x)
		{
			x.matrix_pallet = std::move(data.matrix_pallet);
		}
	};


	struct highlighted_t
	{
	};

	struct selected_t
	{
	};

	core::container::MultiCollection
	<
		engine::Entity,
		201,
		std::array<highlighted_t, 100>,
		std::array<selected_t, 100>
	>
	selected_components;
}

namespace
{
	using namespace engine::graphics::detail;

	core::container::PageQueue<utility::heap_storage<std::string, ShaderData>> queue_shaders;

	FontManager font_manager;
	ShaderManager shader_manager;

	void maybe_resize_framebuffer();
	void poll_queues()
	{
		bool should_maybe_resize_framebuffer = false;
		DisplayMessage display_message;
		while (queue_displays.try_pop(display_message))
		{
			struct ProcessMessage
			{
				bool & should_maybe_resize_framebuffer;

				void operator () (MessageAddDisplay && x)
				{
					debug_verify(displays.try_emplace<display_t>(x.asset,
					                                             x.display.viewport.x, x.display.viewport.y, x.display.viewport.width, x.display.viewport.height,
					                                             x.display.camera_3d.projection, x.display.camera_3d.frame, x.display.camera_3d.view, x.display.camera_3d.inv_projection, x.display.camera_3d.inv_frame, x.display.camera_3d.inv_view,
					                                             x.display.camera_2d.projection, x.display.camera_2d.view));
					should_maybe_resize_framebuffer = true;
				}
				void operator () (MessageRemoveDisplay && x)
				{
					displays.remove(x.asset);
					should_maybe_resize_framebuffer = true;
				}
				void operator () (MessageUpdateDisplayCamera2D && x)
				{
					displays.call(x.asset, update_display_camera_2d{std::move(x.camera_2d)});
				}
				void operator () (MessageUpdateDisplayCamera3D && x)
				{
					displays.call(x.asset, update_display_camera_3d{std::move(x.camera_3d)});
				}
				void operator () (MessageUpdateDisplayViewport && x)
				{
					displays.call(x.asset, update_display_viewport{std::move(x.viewport)});
					should_maybe_resize_framebuffer = true;
				}
			};
			visit(ProcessMessage{should_maybe_resize_framebuffer}, std::move(display_message));
		}
		if (should_maybe_resize_framebuffer)
		{
			maybe_resize_framebuffer();
		}

		AssetMessage asset_message;
		while (queue_assets.try_pop(asset_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageRegisterCharacter && x)
				{
					debug_assert(!resources.contains(x.asset));
					resources.emplace<mesh_t>(x.asset, std::move(x.mesh));
				}
				void operator () (MessageRegisterMesh && x)
				{
					debug_assert(!resources.contains(x.asset));
					resources.emplace<mesh_t>(x.asset, std::move(x.mesh));
				}
				void operator () (MessageRegisterTexture && x)
				{
					debug_assert(!materials.contains(x.asset));
					materials.emplace<texture_t>(x.asset, std::move(x.image));
				}
			};
			visit(ProcessMessage{}, std::move(asset_message));
		}

		EntityMessage entity_message;
		while (queue_entities.try_pop(entity_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageAddBar && x)
				{
					if (components.contains(x.entity))
					{
						// is this safe to do?
						auto & bar = components.get<Bar>(x.entity);
						bar.worldPosition = x.bar.worldPosition;
						bar.progress = x.bar.progress;
					}
					else
					{
						debug_verify(components.try_emplace<Bar>(x.entity, std::move(x.bar)));
					}
				}
				void operator () (MessageAddCharacterT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & mesh = resources.get<mesh_t>(x.component.mesh);
					const auto & texture = materials.get<texture_t>(x.component.texture);
					auto & object = objects.emplace<object_modelview_vertices>(x.entity, mesh, std::move(x.component.modelview));
					debug_verify(components.try_emplace<Character>(x.entity, mesh, texture, object));
					debug_verify(updateable_components.try_emplace<updateable_character_t>(x.entity, mesh, object));
				}
				void operator () (MessageAddComponentC && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.component.modelview));
					debug_verify(components.try_emplace<comp_c>(x.entity, object, std::move(x.component.assets)));
				}
				void operator () (MessageAddComponentT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & mesh = resources.get<mesh_t>(x.component.mesh);
					const auto & texture = materials.get<texture_t>(x.component.texture);
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.component.modelview));
					debug_verify(components.try_emplace<comp_t>(x.entity, mesh, texture, object));
				}
				void operator () (MessageAddLineC && x)
				{
					debug_assert(!components.contains(x.entity));
					debug_verify(components.try_emplace<linec_t>(x.entity, std::move(x.line)));
				}
				void operator () (MessageAddPanelC && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.panel.matrix));
					debug_verify(components.try_emplace<::ui::PanelC>(x.entity, object, std::move(x.panel.size), std::move(x.panel.color)));
				}
				void operator () (MessageAddPanelT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & texture = materials.get<texture_t>(x.panel.texture);
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.panel.matrix));
					debug_verify(components.try_emplace<::ui::PanelT>(x.entity, texture, object, std::move(x.panel.size)));
				}
				void operator () (MessageAddText && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.text.matrix));
					debug_verify(components.try_emplace<::ui::Text>(x.entity, object, std::move(x.text.display), std::move(x.text.color)));
				}
				void operator () (MessageMakeObstruction && x)
				{
					const engine::graphics::opengl::Color4ub color = {0, 0, 0, 0};
					if (selectable_components.contains(x.entity))
					{
						selectable_components.call(x.entity, set_selectable_color{color});
					}
					else
					{
						components.call(x.entity, add_selectable_color{color});
					}
				}
				void operator () (MessageMakeSelectable && x)
				{
					const engine::graphics::opengl::Color4ub color = {(x.entity & 0x000000ff) >> 0,
					                                                  (x.entity & 0x0000ff00) >> 8,
					                                                  (x.entity & 0x00ff0000) >> 16,
					                                                  (x.entity & 0xff000000) >> 24};
					if (selectable_components.contains(x.entity))
					{
						selectable_components.call(x.entity, set_selectable_color{color});
					}
					else
					{
						components.call(x.entity, add_selectable_color{color});
					}
				}
				void operator () (MessageMakeTransparent && x)
				{
					if (selectable_components.contains(x.entity))
					{
						selectable_components.remove(x.entity);
					}
				}
				void operator () (MessageMakeClearSelection && x)
				{
					debug_fail(); // not implemented yet
				}
				void operator () (MessageMakeDehighlighted && x)
				{
					selected_components.try_remove<highlighted_t>(x.entity);
				}
				void operator () (MessageMakeDeselect && x)
				{
					selected_components.try_remove<selected_t>(x.entity);
				}
				void operator () (MessageMakeHighlighted && x)
				{
					selected_components.emplace<highlighted_t>(x.entity);
				}
				void operator () (MessageMakeSelect && x)
				{
					selected_components.emplace<selected_t>(x.entity);
				}
				void operator () (MessageRemove && x)
				{
					debug_assert(components.contains(x.entity));
					if (selectable_components.contains(x.entity))
					{
						selectable_components.remove(x.entity);
					}
					if (updateable_components.contains(x.entity))
					{
						updateable_components.remove(x.entity);
					}
					components.remove(x.entity);
					if (objects.contains(x.entity))
					{
						objects.remove(x.entity);
					}
				}
				void operator () (MessageUpdateCharacterSkinning && x)
				{
					debug_assert(updateable_components.contains(x.entity));
					updateable_components.call(x.entity, update_matrixpallet{std::move(x.character_skinning)});
				}
				void operator () (MessageUpdateModelviewMatrix && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.call(x.entity, update_modelview{std::move(x.modelview_matrix)});
				}
				void operator () (MessageUpdatePanelC && x)
				{
					debug_assert(components.contains(x.entity));
					auto & view = components.get<ui::PanelC>(x.entity);
					view.object->modelview = x.panel.matrix;
					view.color = color_from(x.panel.color);
					view.size = x.panel.size;
				}
				void operator () (MessageUpdatePanelT && x)
				{
					debug_assert(components.contains(x.entity));
					const auto & texture = materials.get<texture_t>(x.panel.texture);
					auto & view = components.get<ui::PanelT>(x.entity);
					view.object->modelview = x.panel.matrix;
					view.size = x.panel.size;
					view.texture = &texture;
				}
				void operator () (MessageUpdateText && x)
				{
					debug_assert(components.contains(x.entity));
					auto & view = components.get<ui::Text>(x.entity);
					view.object->modelview = x.text.matrix;
					view.color = color_from(x.text.color);
					view.display = x.text.display;
				}
			};
			visit(ProcessMessage{}, std::move(entity_message));
		}

		std::pair<std::string, ShaderData> shader_data_pair;
		while (queue_shaders.try_pop(shader_data_pair))
		{
			debug_printline("trying to create \"", shader_data_pair.first, "\"");
			shader_manager.create(engine::Asset(shader_data_pair.first), std::move(shader_data_pair.second));
		}
	}
}

namespace
{
	Stack modelview_matrix;

#if !TEXT_USE_FREETYPE
	engine::graphics::opengl::Font normal_font;
#endif

	int framebuffer_width = 0;
	int framebuffer_height = 0;
	GLuint framebuffer;
	GLuint entitybuffers[2]; // color, depth
	GLuint entitytexture; // color
	std::vector<uint32_t> entitypixels;
	engine::graphics::opengl::Color4ub highlighted_color{255, 191, 64, 255};
	engine::graphics::opengl::Color4ub selected_color{64, 191, 255, 255};

	void resize_framebuffer(int width, int height)
	{
		framebuffer_width = width;
		framebuffer_height = height;

		// free old render buffers
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0); // color
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0); // depth
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		glDeleteTextures(1, &entitytexture);
		glDeleteRenderbuffers(2, entitybuffers);
		// allocate new render buffers
		glGenRenderbuffers(2, entitybuffers);
		glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[0]); // color
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, framebuffer_width, framebuffer_height);
		glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[1]); // depth
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, framebuffer_width, framebuffer_height);

		glGenTextures(1, &entitytexture);
		glBindTexture(GL_TEXTURE_2D, entitytexture);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, framebuffer_width, framebuffer_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, entitybuffers[0]); // color
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, entitytexture, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, entitybuffers[1]); // depth
		const GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		glDrawBuffers(2, buffers);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		entitypixels.resize(framebuffer_width * framebuffer_height);

		debug_assert(glGetError() == GL_NO_ERROR);
	}
	void maybe_resize_framebuffer()
	{
		int width = 0;
		int height = 0;

		for (const auto & display : displays.get<display_t>())
		{
			const int maybe_width = display.x + display.width;
			if (maybe_width > width) width = maybe_width;
			const int maybe_height = display.y + display.height;
			if (maybe_height > height) height = maybe_height;
		}

		if (width != framebuffer_width ||
		    height != framebuffer_height)
		{
			resize_framebuffer(width, height);
		}
	}

	engine::Entity get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = framebuffer_height - 1 - sy;

		if (x >= 0 &&
		    y >= 0 &&
		    x < 0 + framebuffer_width &&
		    y < 0 + framebuffer_height)
		{
			const unsigned int color = entitypixels[x + y * framebuffer_width];
			return engine::Entity{
				(color & 0xff000000) >> 24 |
				(color & 0x00ff0000) >> 8 |
				(color & 0x0000ff00) << 8 |
				(color & 0x000000ff) << 24};
		}
		return engine::Entity::null();
	}

	void initLights()
	{
		// set up light colors (ambient, diffuse, specular)
		GLfloat lightKa[] = { .5f, .5f, .5f, 1.f }; // ambient light
		GLfloat lightKd[] = { 0.f, 0.f, 0.f, 1.f }; // diffuse light
		GLfloat lightKs[] = { 0.f, 0.f, 0.f, 1.f }; // specular light
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

		// position the light
		float lightPos[4] = { 0, 0, 20, 1 }; // positional light
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glEnable(GL_LIGHT0);

		debug_assert(glGetError() == GL_NO_ERROR);
	}

	template <typename T, std::size_t N>
	auto convert(std::array<T, N> && data)
	{
		core::container::Buffer buffer;
		buffer.resize<T>(N);

		std::copy(data.begin(), data.end(), buffer.data_as<T>());

		return buffer;
	}

	// TODO: move to loader/level
	auto createCuboid()
	{
		// the vertices of cuboid are numbered as followed:
		//              23--22
		//             /    /
		//            21--20
		//
		//        6  11--10   15
		//       /|  |    |   /|
		//      7 |  |    |  14|  18--19
		//      | 4  9----8  |13  |    |
		//      |/           |/   |    |
		//      5   0----1   12   16--17
		//   y     /    /
		//   |    2----3
		//   |
		//   +----x
		//  /
		// z
		const float xoffset = .5f;
		const float yoffset = .5f;
		const float zoffset = .5f;

		std::array<float, 3 * 24> va;
		std::size_t i = 0;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;

		core::container::Buffer vertices = convert(std::move(va));
		core::container::Buffer triangles = convert(std::array<uint16_t, 3 * 12> {{
				0, 1, 3,
				0, 3, 2,
				4, 5, 7,
				4, 7, 6,
				8, 9, 11,
				8, 11, 10,
				12, 13, 15,
				12, 15, 14,
				16, 17, 19,
				16, 19, 18,
				20, 21, 23,
				20, 23, 22}});
		core::container::Buffer normals = convert(std::array<float, 3 * 24> {{
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f}});
		core::container::Buffer coords = convert(std::array<float, 2 * 24> {{
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f}});

		return std::move(engine::graphics::data::Mesh{
			vertices, triangles, normals, coords });
	}

	std::atomic_int read_lock;

	struct TryReadImage
	{
		core::graphics::Image & image;

		void operator () (core::PngStructurer && x)
		{
			x.read(image);
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("impossible to read, maybe");
		}
	};

	void data_callback_image(std::string name, engine::resource::reader::Structurer && structurer)
	{
		core::graphics::Image image;
		visit(TryReadImage{image}, std::move(structurer));

		engine::Asset asset = engine::Asset::null();
		if (name == "res/box.png")
			asset = engine::Asset("my_png");
		else
		{
			debug_assert((name[0] == 'r' && name[1] == 'e' && name[2] == 's' && name[3] == '/'));
			debug_assert((name[name.size() - 4] == '.' && name[name.size() - 3] == 'p' && name[name.size() - 2] == 'n' && name[name.size() - 1] == 'g'));
			asset = engine::Asset(name.data() + 4, name.size() - 4 - 4);
		}

		post_register_texture(*self, asset, std::move(image));
		read_lock++;
	}

	struct TryReadShader
	{
		ShaderData & image;

		void operator () (core::ShaderStructurer && x)
		{
			x.read(image);
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("impossible to read, maybe");
		}
	};

	void data_callback_shader(std::string name, engine::resource::reader::Structurer && structurer)
	{
		ShaderData shader_data;
		visit(TryReadShader{shader_data}, std::move(structurer));

		queue_shaders.try_push(std::make_pair(std::move(name), std::move(shader_data)));
		read_lock++;
	}

#if TEXT_USE_FREETYPE
	struct TryReadTtf
	{
		std::vector<char> & data;

		void operator () (core::BytesStructurer && x)
		{
			x.read(data);
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("impossible to read, maybe");
		}
	};

	void data_callback_ttf(std::string name, engine::resource::reader::Structurer && structurer)
	{
		std::vector<char> data;
		visit(TryReadTtf{data}, std::move(structurer));

		debug_printline("<<>><<>><<>><<>><<>><<>><<>><<>><<>><<>><<>>");

		FT_Library library;
		if (FT_Init_FreeType(&library))
			debug_fail();

		FT_Face face;
		if (FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(data.data()), data.size(), 0, &face))
		{
			debug_fail();
			goto cleanup_library;
		}
		debug_printline(name, ": face->num_glyphs = ", face->num_glyphs);
		debug_printline(name, ": face has kerning = ", FT_HAS_KERNING(face));

		if (FT_Select_Charmap(face, FT_ENCODING_UNICODE))
		{
			debug_fail();
			goto cleanup_face;
		}

		if (FT_Set_Pixel_Sizes(face, 0, 24))
		{
			debug_fail();
			goto cleanup_face;
		}

		{
			std::vector<utility::unicode_code_point> unicode_indices;
			unicode_indices.reserve(face->num_glyphs);
			std::vector<int> glyph_indices;
			glyph_indices.reserve(face->num_glyphs);
			{
				FT_UInt glyph_index;
				FT_UInt unicode_index = FT_Get_First_Char(face, &glyph_index);
				while (glyph_index != 0)
				{
					unicode_indices.emplace_back(unicode_index);
					glyph_indices.push_back(glyph_index);

					unicode_index = FT_Get_Next_Char(face, unicode_index, &glyph_index);
				}
				glyph_indices.push_back(0);
				debug_assert(unicode_indices.size() + 1 == glyph_indices.size());
				debug_printline(name, ": face charmap size = ", unicode_indices.size());
			}

			const int total_slots = glyph_indices.size();


			int maxx = 0, maxy = 0;
			std::vector<FontManager::SymbolData> symbol_data(total_slots);
			for (int i = 0; i < total_slots; i++)
			{
				const int glyph_index = glyph_indices[i];
				if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER))
				{
					debug_fail();
					continue;
				}

				maxx = std::max(maxx, static_cast<int>(face->glyph->bitmap.width));
				maxy = std::max(maxy, static_cast<int>(face->glyph->bitmap.rows));

				symbol_data[i].offset_x = static_cast<int16_t>(face->glyph->bitmap_left);
				debug_assert(symbol_data[i].offset_x == face->glyph->bitmap_left, "16 bits are not enough for offset, should we use 32?");
				symbol_data[i].offset_y = static_cast<int16_t>(face->glyph->bitmap.rows - face->glyph->bitmap_top);
				debug_assert(symbol_data[i].offset_y == face->glyph->bitmap.rows - face->glyph->bitmap_top, "16 bits are not enough for offset, should we use 32?");
				symbol_data[i].advance_x = static_cast<int16_t>(face->glyph->advance.x >> 6);
				debug_assert(symbol_data[i].advance_x == face->glyph->advance.x >> 6, "16 bits are not enough for advancement, should we use 26?");
				symbol_data[i].advance_y = static_cast<int16_t>(face->glyph->advance.y >> 6);
				debug_assert(symbol_data[i].advance_y == face->glyph->advance.y >> 6, "16 bits are not enough for advancement, should we use 26?");
			}
			debug_printline(name, ": face max = {", maxx, ", ", maxy, "}");
			debug_assert(maxx > 0);
			debug_assert(maxy > 0);


			const int border_size = 1;
			const int slot_size_x = maxx + border_size * 2;
			const int slot_size_y = maxy + border_size * 2;

			std::vector<std::tuple<int, int, int, int>> texture_dimensions;
			for (int texture_height = utility::clp2(slot_size_y);; texture_height *= 2)
			{
				const int max_in_y = texture_height / slot_size_y;
				const int needed_in_x = (total_slots + max_in_y - 1) / max_in_y;

				const int texture_width = utility::clp2(needed_in_x * slot_size_x);
				const int max_in_x = texture_width / slot_size_x;

				const int unused_slots = max_in_x * max_in_y - total_slots;

				texture_dimensions.push_back({std::abs(texture_width - texture_height), unused_slots, texture_width, texture_height});

				if (max_in_y >= total_slots)
					break;
			}
			std::sort(texture_dimensions.begin(), texture_dimensions.end());

			for (const auto & dim : texture_dimensions)
			{
				debug_printline(name, ": face texture dimension = {", std::get<2>(dim), ", ", std::get<3>(dim), "}, unused slots = ", std::get<1>(dim), "(", static_cast<int>(static_cast<double>(std::get<1>(dim) * slot_size_x * slot_size_y) / static_cast<double>(std::get<2>(dim) * std::get<3>(dim)) * 100.), "%)");
			}


			const int texture_width = std::get<2>(texture_dimensions.front());
			const int texture_height = std::get<3>(texture_dimensions.front());
			const int slots_in_x = texture_width / slot_size_x;
			const int slots_in_y = texture_height / slot_size_y;


			core::container::Buffer pixels(core::container::Buffer::Format::float32, texture_width * texture_height);
			std::vector<float> distance_field(slot_size_x * slot_size_y);
			const double furthest_d = std::sqrt(slot_size_x * slot_size_x + slot_size_y * slot_size_y);
			for (int i = 0; i < total_slots; i++)
			{
				const int glyph_index = glyph_indices[i];
				if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER))
				{
					debug_fail();
					continue;
				}

				const unsigned char * const bitmap_buffer = face->glyph->bitmap.pitch < 0 ? face->glyph->bitmap.buffer : face->glyph->bitmap.buffer + face->glyph->bitmap.pitch * (face->glyph->bitmap.rows - 1);
				const auto sample_at =
					[&](int x, int y)
					{
						return
							(x >= border_size && static_cast<unsigned int>(x) < border_size + face->glyph->bitmap.width) &&
							(y >= border_size && static_cast<unsigned int>(y) < border_size + face->glyph->bitmap.rows) ?
							bitmap_buffer[(x - border_size) - (y - border_size) * face->glyph->bitmap.pitch] >= 128 : 0;
					};
				std::fill(distance_field.begin(), distance_field.end(), static_cast<float>(furthest_d));
				// for (int y = 0; y < slot_size_y; y++)
				for (int y : ranges::index_sequence(face->glyph->bitmap.rows + 2 * border_size))
				{
					// for (int x = 0; x < slot_size_x; x++)
					for (int x : ranges::index_sequence(face->glyph->bitmap.width + 2 * border_size))
					{
						const auto sample = sample_at(x, y);

						double closest_dsq = slot_size_x * slot_size_x + slot_size_y * slot_size_y;
						// for (int t = 0; t < slot_size_y; t++)
						for (int t : ranges::index_sequence(face->glyph->bitmap.rows + 2 * border_size))
						{
							// for (int s = 0; s < slot_size_x; s++)
							for (int s : ranges::index_sequence(face->glyph->bitmap.width + 2 * border_size))
							{
								if (sample_at(s, t) == sample)
									continue;

								const double dx = s - x;
								const double dy = t - y;
								const double dsq = dx * dx + dy * dy;
								closest_dsq = std::min(dsq, closest_dsq);
							}
						}
						distance_field[x + y * slot_size_x] = static_cast<float>(((sample == 0 ? 1 : -1) * std::sqrt(closest_dsq) + furthest_d) / (2. * furthest_d));
					}
				}

				const int slot_index = i;
				const int slot_x = slot_index % slots_in_x;
				const int slot_y = slot_index / slots_in_x;
				const int pixel_offset = slot_x * slot_size_x + (slot_y * slot_size_y) * texture_width;
				float * const pixel_data = pixels.data_as<float>() + pixel_offset;
				for (int y = 0; y < slot_size_y; y++)
				{
					for (int x = 0; x < slot_size_x; x++)
					{
						pixel_data[x + y * texture_width] = distance_field[x + y * slot_size_x];
					}
				}
			}

			const auto asset = engine::Asset(name);
			post_register_texture(*self, asset, core::graphics::Image(texture_width, texture_height, core::graphics::BitDepth::eight, core::graphics::ChannelCount::one, core::graphics::ColorType::R, std::move(pixels)));

			font_manager.create(std::move(name), std::move(unicode_indices), std::move(symbol_data), slot_size_x, slot_size_y, texture_width, texture_height);
		}

	cleanup_face:
		FT_Done_Face(face);
	cleanup_library:
		FT_Done_FreeType(library);

		debug_printline("<<>><<>><<>><<>><<>><<>><<>><<>><<>><<>><<>>");
		read_lock++;
	}
#endif

	void render_setup()
	{
		debug_printline(engine::graphics_channel, "render_callback starting");
		make_current(*engine::graphics::detail::window);

		debug_printline(engine::graphics_channel, "glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		debug_printline(engine::graphics_channel, "glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		debug_printline(engine::graphics_channel, "glGetString GL_VERSION: ", glGetString(GL_VERSION));

		engine::graphics::opengl::init();

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		initLights();

		// entity buffers
		glGenFramebuffers(1, &framebuffer);
		glGenRenderbuffers(2, entitybuffers);
		glGenTextures(1, &entitytexture);

		// TODO: move to loader/level
		// vvvvvvvv tmp vvvvvvvv
		read_lock = 0;
#if TEXT_USE_FREETYPE
		reader->post_read("res/font/consolas.ttf", data_callback_ttf);
#else
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load(*engine::graphics::renderer::window, "consolas", 14))
			{
				debug_fail();
			}
			normal_font.compile(data);

			data.free();
		}
#endif

		// add cuboid mesh as an asset
		post_register_mesh(*self, engine::Asset{"cuboid"}, createCuboid());

		reader->post_read("res/box.png", data_callback_image);
		reader->post_read("res/dude.png", data_callback_image);
		reader->post_read("res/photo.png", data_callback_image);

		post_add_component(
			*self,
			engine::Entity::create(),
			engine::graphics::data::CompT{
				core::maths::Matrix4x4f::translation(0.f, 5.f, 0.f),
				core::maths::Vector3f{1.f, 1.f, 1.f},
				engine::Asset{ "cuboid" },
				engine::Asset{ "my_png" } });

		reader->post_read("res/gfx/color.130.glsl", data_callback_shader);
		reader->post_read("res/gfx/entity.130.glsl", data_callback_shader);
		reader->post_read("res/gfx/text.130.glsl", data_callback_shader);
		reader->post_read("res/gfx/texture.130.glsl", data_callback_shader);

#if TEXT_USE_FREETYPE
		while (read_lock < 8);
#else
		while (read_lock < 7);
#endif
		// ^^^^^^^^ tmp ^^^^^^^^
	}

	void render_update()
	{
		// poll events
		poll_queues();
		// update components
		for (auto & component : updateable_components.get<updateable_character_t>())
		{
			component.update();
		}

		if (framebuffer_width == 0 || framebuffer_height == 0)
			return;

		const GLint p_color = shader_manager.get(engine::Asset("res/gfx/color.130.glsl"));
		const GLint p_entity = shader_manager.get(engine::Asset("res/gfx/entity.130.glsl"));
		const GLint p_tex = shader_manager.get(engine::Asset("res/gfx/texture.130.glsl"));
		const GLint p_text = shader_manager.get(engine::Asset("res/gfx/text.130.glsl"));

		glStencilMask(0x000000ff);
		// setup frame
		glClearColor(0.f, 0.f, .1f, 0.f);
		glClearDepth(1.);
		//glClearStencil(0x00000000);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		debug_assert(glGetError() == GL_NO_ERROR);

		////////////////////////////////////////////////////////
		//
		//  entity buffer
		//
		////////////////////////////////////////
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearColor(0.f, 0.f, 0.f, 0.f); // null entity
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		debug_assert(glGetError() == GL_NO_ERROR);

		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);

		for (const auto & display : displays.get<display_t>())
		{
			glViewport(display.x, display.y, display.width, display.height);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_3d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_3d);

		if (p_entity >= 0)
		{
		glUseProgram(p_entity);
		glUniform(p_entity, "projection_matrix", display.projection_3d);
		for (const auto & component : selectable_components.get<selectable_character_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_entity, "modelview_matrix", modelview_matrix.top());

			const auto color_location = 5;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(color_location, component.selectable_color[0] / 255.f, component.selectable_color[1] / 255.f, component.selectable_color[2] / 255.f, component.selectable_color[3] / 255.f);

			const auto vertex_location = 4;// glGetAttribLocation(p_tex, "in_vertex");
			glEnableVertexAttribArray(vertex_location);
			glVertexAttribPointer(
				vertex_location,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				component.object->vertices.data());
			glDrawElements(
				GL_TRIANGLES,
				component.mesh->triangles.count(),
				GL_UNSIGNED_SHORT, // TODO
				component.mesh->triangles.data());
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		for (const auto & component : selectable_components.get<selectable_comp_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_entity, "modelview_matrix", modelview_matrix.top());

			const auto color_location = 5;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(color_location, component.selectable_color[0] / 255.f, component.selectable_color[1] / 255.f, component.selectable_color[2] / 255.f, component.selectable_color[3] / 255.f);

			const auto vertex_location = 4;// glGetAttribLocation(p_tex, "in_vertex");
			glEnableVertexAttribArray(vertex_location);
			for (const auto & mesh : component.meshes)
			{
				glVertexAttribPointer(
					vertex_location,
					3,
					BufferFormats[static_cast<int>(mesh->vertices.format())],
					GL_FALSE,
					0,
					mesh->vertices.data());
				glDrawElements(
					GL_TRIANGLES,
					mesh->triangles.count(),
					BufferFormats[static_cast<int>(mesh->triangles.format())],
					mesh->triangles.data());
			}
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		for (const auto & component : selectable_components.get<selectable_comp_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_entity, "modelview_matrix", modelview_matrix.top());

			const auto color_location = 5;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(color_location, component.selectable_color[0] / 255.f, component.selectable_color[1] / 255.f, component.selectable_color[2] / 255.f, component.selectable_color[3] / 255.f);

			const auto vertex_location = 4;// glGetAttribLocation(p_tex, "in_vertex");
			glEnableVertexAttribArray(vertex_location);
			glVertexAttribPointer(
				vertex_location,
				3,
				BufferFormats[static_cast<int>(component.mesh->vertices.format())],
				GL_FALSE,
				0,
				component.mesh->vertices.data());
			glDrawElements(
				GL_TRIANGLES,
				component.mesh->triangles.count(),
				BufferFormats[static_cast<int>(component.mesh->triangles.format())],
				component.mesh->triangles.data());
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		}

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_2d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_2d);

		if (p_entity >= 0)
		{
		glUniform(p_entity, "projection_matrix", display.projection_2d);
		for (const auto & component : selectable_components.get<selectable_panel>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_entity, "modelview_matrix", modelview_matrix.top());

			const auto color_location = 5;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(color_location, component.selectable_color[0] / 255.f, component.selectable_color[1] / 255.f, component.selectable_color[2] / 255.f, component.selectable_color[3] / 255.f);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			const GLfloat vertices[] = {
				0.f, size[1],
				size[0], size[1],
				size[0], 0.f,
				0.f, 0.f
			};
			const GLushort indices[] = {
				0, 1, 2,
				2, 3, 0
			};

			const auto vertex_location = 4;// glGetAttribLocation(p_tex, "in_vertex");
			glEnableVertexAttribArray(vertex_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				vertices);
			glDrawElements(
				GL_TRIANGLES,
				6,
				GL_UNSIGNED_SHORT,
				indices);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}

		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

		glReadPixels(0, 0, framebuffer_width, framebuffer_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, entitypixels.data());

		// select
		{
			std::tuple<int, int, engine::Entity, engine::Command> select_args;
			while (queue_select.try_pop(select_args))
			{
				engine::graphics::renderer::SelectData select_data = {get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)), {std::get<0>(select_args), std::get<1>(select_args)}};
				callback_select(std::get<2>(select_args), std::get<3>(select_args), std::move(select_data));
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		//////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////

		for (const auto & display : displays.get<display_t>())
		{
			glViewport(display.x, display.y, display.width, display.height);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_3d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_3d);

		// wireframes
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glDisable(GL_LIGHTING);

		glStencilMask(0x00000001);
		// glStencilFunc(GL_NEVER, 0x00000001, 0x00000001);
		// glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0x00000001, 0x00000001);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		// TODO: make it possible to draw wireframes using meshes if needed.

		glEnable(GL_LIGHTING);

		//////////////////////////////////////////////////////////////
		// solids
		glStencilMask(0x00000000);
		glStencilFunc(GL_EQUAL, 0x00000000, 0x00000001);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		if (p_tex >= 0)
		{
		glUseProgram(p_tex);
		glUniform(p_tex, "projection_matrix", display.projection_3d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_tex, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_tex, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (auto & component : components.get<Character>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			modelview_matrix.mult(component.mesh->modelview);
			glUniform(p_tex, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, entitytexture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, component.texture->id);

			glUniform(p_tex, "tex", 0);
			glUniform(p_tex, "entitytex", 1);

			const auto vertex_location = 5;// glGetAttribLocation(p_tex, "in_vertex");
			const auto normal_location = 6;// glGetAttribLocation(p_tex, "in_normal");
			const auto texcoord_location = 7;// glGetAttribLocation(p_tex, "in_texcoord");
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(normal_location);
			glEnableVertexAttribArray(texcoord_location);
			glVertexAttribPointer(
				vertex_location,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				component.object->vertices.data());
			glVertexAttribPointer(
				normal_location,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				component.mesh->normals.data());
			glVertexAttribPointer(
				texcoord_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				component.mesh->coords.data());
			glDrawElements(
				GL_TRIANGLES,
				component.mesh->triangles.count(),
				GL_UNSIGNED_SHORT, // TODO
				component.mesh->triangles.data());
			glDisableVertexAttribArray(texcoord_location);
			glDisableVertexAttribArray(normal_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
		if (p_color >= 0)
		{
		glUseProgram(p_color);
		glUniform(p_color, "projection_matrix", display.projection_3d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_color, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_color, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const auto & component : components.get<linec_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glUniform(p_color, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;// glGetAttribLocation(p_tex, "status_flags");
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			const auto color_location = 7;
			glVertexAttrib4f(color_location, component.color[0] / 255.f, component.color[1] / 255.f, component.color[2] / 255.f, component.color[3] / 255.f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, entitytexture);

			glUniform(p_color, "entitytex", 0);

			glLineWidth(2.f);

			const auto vertex_location = 5;
			glEnableVertexAttribArray(vertex_location);
			glVertexAttribPointer(
				vertex_location,
				3,
				BufferFormats[static_cast<int>(component.vertices.format())],
				GL_FALSE,
				0,
				component.vertices.data());
			glDrawElements(
				GL_LINES,
				component.edges.count(),
				BufferFormats[static_cast<int>(component.edges.format())],
				component.edges.data());
			glDisableVertexAttribArray(vertex_location);

			glLineWidth(1.f);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
		if (p_tex >= 0)
		{
		glUseProgram(p_tex);
		glUniform(p_tex, "projection_matrix", display.projection_3d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_tex, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_tex, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const auto & component : components.get<comp_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_tex, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, entitytexture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, component.texture->id);

			glUniform(p_tex, "tex", 0);
			glUniform(p_tex, "entitytex", 1);

			const mesh_t & mesh = *component.mesh;

			const auto vertex_location = 5;
			const auto normal_location = 6;
			const auto texcoord_location = 7;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(normal_location);
			glEnableVertexAttribArray(texcoord_location);
			glVertexAttribPointer(
				vertex_location,
				3,
				BufferFormats[static_cast<int>(mesh.vertices.format())],
				GL_FALSE,
				0,
				mesh.vertices.data());
			glVertexAttribPointer(
				normal_location,
				3,
				BufferFormats[static_cast<int>(mesh.normals.format())],
				GL_FALSE,
				0,
				mesh.normals.data());
			glVertexAttribPointer(
				texcoord_location,
				2,
				BufferFormats[static_cast<int>(mesh.coords.format())],
				GL_FALSE,
				0,
				mesh.coords.data());
			glDrawElements(
				GL_TRIANGLES,
				mesh.triangles.count(),
				BufferFormats[static_cast<int>(mesh.triangles.format())],
				mesh.triangles.data());
			glDisableVertexAttribArray(texcoord_location);
			glDisableVertexAttribArray(normal_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
		if (p_color >= 0)
		{
		glUseProgram(p_color);
		glUniform(p_color, "projection_matrix", display.projection_3d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_color, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_color, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const comp_c & component : components.get<comp_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_color, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, entitytexture);

			glUniform(p_color, "entitytex", 0);

			const auto vertex_location = 5;
			const auto normal_location = 6;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(normal_location);
			for (const auto & a : component.assets)
			{
				const mesh_t & mesh = *a.mesh;

				const auto color_location = 7;
				glVertexAttrib4f(color_location, a.color[0] / 255.f, a.color[1] / 255.f, a.color[2] / 255.f, a.color[3] / 255.f);

				glVertexAttribPointer(
					vertex_location,
					3,
					BufferFormats[static_cast<int>(mesh.vertices.format())],
					GL_FALSE,
					0,
					mesh.vertices.data());
				glVertexAttribPointer(
					normal_location,
					3,
					BufferFormats[static_cast<int>(mesh.normals.format())],
					GL_FALSE,
					0,
					mesh.normals.data());
				glDrawElements(
					GL_TRIANGLES,
					mesh.triangles.count(),
					BufferFormats[static_cast<int>(mesh.triangles.format())],
					mesh.triangles.data());
			}
			glDisableVertexAttribArray(normal_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_2d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_2d);

		// disable depth test to make the bar drawings show above the 3d content
		glDisable(GL_DEPTH_TEST);

		// 2d
		// ...
		if (p_color >= 0)
		{
		glUseProgram(p_color);
		glUniform(p_color, "projection_matrix", display.projection_2d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_color, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_color, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const Bar & component : components.get<Bar>())
		{
			modelview_matrix.push();

			// calculate modelview of bar position in world space
			core::maths::Vector2f frameCoord = display.from_world_to_frame(component.worldPosition);
			core::maths::Vector2f::array_type b;
			frameCoord.get_aligned(b);

			core::maths::Matrix4x4f modelview = make_translation_matrix(core::maths::Vector3f{ b[0], b[1], 0.f });

			modelview_matrix.mult(modelview);
			glUniform(p_color, "modelview_matrix", modelview_matrix.top());

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, 0.f, 0.f, 0.f, 0.f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, entitytexture);

			glUniform(p_color, "entitytex", 0);

			const float WF = 28.f;
			const float WI = WF - 2.f;
			const float HF = 5.f;
			const float HI = HF - 2.f;
			const float ws = -WI + component.progress * WI * 2;

			const GLfloat vertices[] = {
				-WF, -HF,
				-WF, HF,
				WF, HF,
				WF, -HF,

				-WI, -HI,
				-WI, HI,
				ws, HI,
				ws, -HI
			};
			const GLubyte colors[] = {
				20, 20, 40, 255,
				20, 20, 40, 255,
				20, 20, 40, 255,
				20, 20, 40, 255,

				100, 255, 80, 255,
				100, 255, 80, 255,
				100, 255, 80, 255,
				100, 255, 80, 255
			};
			const GLushort indices[] = {
				0, 1, 2,
				2, 3, 0,

				4, 5, 6,
				6, 7, 4
			};

			const auto vertex_location = 5;
			const auto color_location = 7;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(color_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				vertices);
			glVertexAttribPointer(
				color_location,
				4,
				GL_UNSIGNED_BYTE,
				GL_TRUE,
				0,
				colors);
			glDrawElements(
				GL_TRIANGLES,
				12,
				GL_UNSIGNED_SHORT,
				indices);
			glDisableVertexAttribArray(color_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}


		// clear depth to make GUI show over all prev. rendering
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);

		// draw gui
#if TEXT_USE_FREETYPE
		if (p_text >= 0 && materials.contains<texture_t>(engine::Asset("res/font/consolas.ttf")) && font_manager.has(engine::Asset("res/font/consolas.ttf")))
		{
		glUseProgram(p_text);
		glUniform(p_text, "projection_matrix", display.projection_2d);
		{
			core::container::Buffer vertices;
			core::container::Buffer texcoords;
			font_manager.compile(engine::Asset("res/font/consolas.ttf"), u8"herp derp herp derp \u00c5\u00c4\u00d6 \u00e5\u00e4\u00f6 \u03b1\u03b2\u03b3\u03b4\u03b5\u03b6\u03b7 \u0410\u0411\u0412\u0413\u0414\u0415\u0416 \u2603.", vertices, texcoords);
			debug_assert(vertices.count() / 8 == 46);
			debug_assert(texcoords.count() / 8 == 46);
			debug_assert(vertices.format() == core::container::Buffer::Format::float32);
			debug_assert(texcoords.format() == core::container::Buffer::Format::float32);

			const auto & texture = materials.get<texture_t>(engine::Asset("res/font/consolas.ttf"));

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.push();
			modelview_matrix.rotate(core::maths::degreef(10.f), 0.f, 0.f, -1.f);
			modelview_matrix.translate(20.f, 160.f, 0.f);
			glUniform(p_text, "modelview_matrix", modelview_matrix.top());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture.id);

			glUniform(p_text, "tex", 0);

			const auto vertex_location = 5;
			const auto texcoord_location = 7;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(texcoord_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				BufferFormats[static_cast<int>(vertices.format())],
				GL_FALSE,
				0,
				vertices.data());
			glVertexAttribPointer(
				texcoord_location,
				2,
				BufferFormats[static_cast<int>(texcoords.format())],
				GL_FALSE,
				0,
				texcoords.data());
			glDrawArrays(
				GL_QUADS, // deprecated
				0,
				vertices.count() / 2);
			glDisableVertexAttribArray(texcoord_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
#else
		{
			glLoadMatrix(modelview_matrix);
			glColor3ub(255, 255, 0);
			normal_font.draw(10, 10 + 12, "herp derp herp derp herp derp herp derp herp derp etc.");
		}
#endif

		if (p_color >= 0)
		{
		glUseProgram(p_color);
		glUniform(p_color, "projection_matrix", display.projection_2d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_color, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_color, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const auto & component : ::components.get<::ui::PanelC>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_color, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			const auto color_location = 7;
			glVertexAttrib4f(color_location, component.color[0] / 255.f, component.color[1] / 255.f, component.color[2] / 255.f, component.color[3] / 255.f);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, entitytexture);

			glUniform(p_color, "entitytex", 0);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			const GLfloat vertices[] = {
				0.f, 0.f,
				0.f, size[1],
				size[0], size[1],
				size[0], 0.f
			};
			const GLushort indices[] = {
				0, 1, 2,
				2, 3, 0
			};

			const auto vertex_location = 5;
			glEnableVertexAttribArray(vertex_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				vertices);
			glDrawElements(
				GL_TRIANGLES,
				6,
				GL_UNSIGNED_SHORT,
				indices);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
		if (p_tex >= 0)
		{
		glUseProgram(p_tex);
		glUniform(p_tex, "projection_matrix", display.projection_2d);
		{
			static int frame_count = 0;
			frame_count++;

			glUniform(p_tex, "time", static_cast<float>(frame_count) / 50.f);
		}
		glUniform(p_tex, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));
		for (const auto & component : ::components.get<::ui::PanelT>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_tex, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);
			const bool is_interactible = selectable_components.contains(entity);

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			const auto normal_location = 6;
			glVertexAttrib4f(normal_location, 0.f, 0.f, 1.f, 0.f);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, entitytexture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, component.texture->id);

			glUniform(p_tex, "tex", 0);
			glUniform(p_tex, "entitytex", 1);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			const GLfloat vertices[] = {
				0.f, 0.f,
				0.f, size[1],
				size[0], size[1],
				size[0], 0.f
			};
			const GLfloat texcoords[] = {
				0.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				1.f, 1.f
			};
			const GLushort indices[] = {
				0, 1, 2,
				2, 3, 0
			};

			const auto vertex_location = 5;
			const auto texcoord_location = 7;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(texcoord_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				vertices);
			glVertexAttribPointer(
				texcoord_location,
				2,
				GL_FLOAT,
				GL_FALSE,
				0,
				texcoords);
			glDrawElements(
				GL_TRIANGLES,
				6,
				GL_UNSIGNED_SHORT,
				indices);
			glDisableVertexAttribArray(texcoord_location);
			glDisableVertexAttribArray(vertex_location);

			debug_assert(glGetError() == GL_NO_ERROR);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}

#if TEXT_USE_FREETYPE
		if (p_text >= 0 && materials.contains<texture_t>(engine::Asset("res/font/consolas.ttf")) && font_manager.has(engine::Asset("res/font/consolas.ttf")))
		{
		glUseProgram(p_text);
		glUniform(p_text, "projection_matrix", display.projection_2d);
		for (const ::ui::Text & component : ::components.get<::ui::Text>())
		{
			core::container::Buffer vertices;
			core::container::Buffer texcoords;
			font_manager.compile(engine::Asset("res/font/consolas.ttf"), component.display.c_str(), vertices, texcoords);

			const auto & texture = materials.get<texture_t>(engine::Asset("res/font/consolas.ttf"));

			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glUniform(p_text, "modelview_matrix", modelview_matrix.top());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture.id);

			glUniform(p_text, "tex", 0);

			const auto vertex_location = 5;
			const auto texcoord_location = 7;
			glEnableVertexAttribArray(vertex_location);
			glEnableVertexAttribArray(texcoord_location);
			glVertexAttribPointer(
				vertex_location,
				2,
				BufferFormats[static_cast<int>(vertices.format())],
				GL_FALSE,
				0,
				vertices.data());
			glVertexAttribPointer(
				texcoord_location,
				2,
				BufferFormats[static_cast<int>(texcoords.format())],
				GL_FALSE,
				0,
				texcoords.data());
			glDrawArrays(
				GL_QUADS, // deprecated
				0,
				vertices.count() / 2);
			glDisableVertexAttribArray(texcoord_location);
			glDisableVertexAttribArray(vertex_location);

			modelview_matrix.pop();
		}
		glUseProgram(0);
		}
#else
		for (const ::ui::Text & component : ::components.get<::ui::Text>())
		{
			core::maths::Vector4f vec = component.object->modelview.get_column<3>();
			core::maths::Vector4f::array_type pos;
			vec.get_aligned(pos);

			modelview_matrix.push();
			modelview_matrix.translate(pos[0], pos[1], pos[2]);
			glLoadMatrix(modelview_matrix);

			glColor(component.color);
			normal_font.draw(0, 0, component.display);

			modelview_matrix.pop();
		}
#endif

		}

		// entity buffers
		if (entitytoggle.load(std::memory_order_relaxed))
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

			glBlitFramebuffer(0, 0, framebuffer_width, framebuffer_height, 0, 0, framebuffer_width, framebuffer_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		// swap buffers
		swap_buffers(*engine::graphics::detail::window);
	}

	void render_teardown()
	{
		debug_printline(engine::graphics_channel, "render_callback stopping");
#if TEXT_USE_FREETYPE
		font_manager.destroy(engine::Asset("res/font/consolas.ttf"));
#else
		// vvvvvvvv tmp vvvvvvvv
		{
			normal_font.decompile();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
#endif
		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);

		engine::Asset resources_not_unregistered[resources.max_size()];
		const int resource_count = resources.get_all_keys(resources_not_unregistered, resources.max_size());
		debug_printline(engine::asset_channel, resource_count, " resources not unregistered:");
		for (int i = 0; i < resource_count; i++)
		{
			debug_printline(engine::asset_channel, resources_not_unregistered[i]);
		}

		engine::Asset materials_not_unregistered[materials.max_size()];
		const int material_count = materials.get_all_keys(materials_not_unregistered, materials.max_size());
		debug_printline(engine::asset_channel, material_count, " materials not unregistered:");
		for (int i = 0; i < material_count; i++)
		{
			debug_printline(engine::asset_channel, materials_not_unregistered[i]);
		}
	}
}

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			namespace opengl_30
			{
				void run()
				{
					render_setup();

					event.wait();
					event.reset();

					while (active.load(std::memory_order_relaxed))
					{
						render_update();

						event.wait();
						event.reset();
					}

					render_teardown();
				}
			}
		}
	}
}
