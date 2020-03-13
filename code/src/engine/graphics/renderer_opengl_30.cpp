
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
#include "core/file/paths.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"
#include "core/PngStructurer.hpp"
#include "core/serialization.hpp"
#include "core/ShaderStructurer.hpp"
#include "core/sync/Event.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/viewer.hpp"

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

			extern core::container::PageQueue<utility::heap_storage<Message>> message_queue;

			extern core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			extern std::atomic<int> entitytoggle;

			extern engine::graphics::renderer * self;
			extern engine::application::window * window;
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

#if !TEXT_USE_FREETYPE
		friend void glLoadMatrix(const Stack & stack)
		{
			glLoadMatrix(stack.stack.top());
		}
#endif
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
		engine::graphics::data::camera_2d && data;

		void operator () (display_t & x)
		{
			x.projection_2d = data.projection;
			x.view_2d = data.view;
		}
	};

	struct update_display_camera_3d
	{
		engine::graphics::data::camera_3d && data;

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
		engine::graphics::data::viewport && data;

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

		std::ptrdiff_t find(engine::Asset asset) const
		{
			return std::find(assets, assets + count, asset) - assets;
		}

		bool has(engine::Asset asset) const
		{
			return find(asset) < count;
		}

		void compile(engine::Asset asset, utility::string_view_utf8 text, core::container::Buffer & vertices, core::container::Buffer & texcoords)
		{
			const auto index = find(asset);
			debug_assert(index < count, "font asset ", asset, " does not exist");

			const FontInfo & info = infos[index];

			const std::ptrdiff_t len = utility::point_difference(text.length()).get();
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
				const int slot = static_cast<int>((maybe == info.allowed_unicodes.end() || *maybe != cp ? info.allowed_unicodes.end() : maybe) - info.allowed_unicodes.begin());
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
			const auto index = find(asset);
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
			const auto index = find(asset);
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

		explicit mesh_t(engine::graphics::data::MeshAsset && data)
			: modelview(core::maths::Matrix4x4f::identity())
			, vertices(std::move(data.vertices))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.coords))
		{}

		explicit mesh_t(engine::model::mesh_t && data)
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

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<Character>
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

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<selectable_character_t>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		void operator () (engine::Entity entity, Character & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_character_t>(entity, x.mesh, x.object, color));
		}

		template <typename T>
		void operator () (engine::Entity /*entity*/, T &)
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
			for (std::ptrdiff_t i : ranges::index_sequence(object->vertices.count() / 3))
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
		engine::graphics::data::CharacterSkinning && data;

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

	core::container::PageQueue<utility::heap_storage<engine::Asset, ShaderData>> queue_shaders;

	FontManager font_manager;
	ShaderManager shader_manager;

	void maybe_resize_framebuffer();
	void poll_queues()
	{
		bool should_maybe_resize_framebuffer = false;

		Message message;
		while (message_queue.try_pop(message))
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

				void operator () (MessageMakeClearSelection &&)
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
			};
			visit(ProcessMessage{should_maybe_resize_framebuffer}, std::move(message));
		}
		if (should_maybe_resize_framebuffer)
		{
			maybe_resize_framebuffer();
		}

		std::pair<engine::Asset, ShaderData> shader_data_pair;
		while (queue_shaders.try_pop(shader_data_pair))
		{
			shader_manager.create(shader_data_pair.first, std::move(shader_data_pair.second));
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

		const GLint p_entity = shader_manager.get(engine::Asset("res/gfx/entity.130.glsl"));
		const GLint p_tex = shader_manager.get(engine::Asset("res/gfx/texture.130.glsl"));

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
				debug_cast<GLsizei>(component.mesh->triangles.count()),
				GL_UNSIGNED_SHORT, // TODO
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
				engine::graphics::data::SelectData select_data = {get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)), {std::get<0>(select_args), std::get<1>(select_args)}};
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
				debug_cast<GLsizei>(component.mesh->triangles.count()),
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

		// clear depth to make GUI show over all prev. rendering
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);

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

				void shader_callback(core::ReadStream && stream, utility::any & /*data*/, engine::Asset debug_expression(match))
				{
					if (!debug_assert(match == engine::Asset(".glsl")))
						return;

					const auto filename = core::file::filename(stream.filepath());
					const auto asset = engine::Asset(filename.data(), filename.size());

					ShaderData shader;
					core::ShaderStructurer structurer(std::move(stream));
					structurer.read(shader);

					queue_shaders.try_emplace(asset, std::move(shader));
				}
			}
		}
	}
}
