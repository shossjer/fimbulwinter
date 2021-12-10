#include "config.h"

#if GRAPHICS_USE_OPENGL

#include "opengl.hpp"
#include "opengl/Color.hpp"
#if !TEXT_USE_FREETYPE
# include "opengl/Font.hpp"
#endif
#include "opengl/Matrix.hpp"

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

#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/HashTable.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/any.hpp"
#include "utility/functional/utility.hpp"
#include "utility/lookup_table.hpp"
#include "utility/profiling.hpp"
#include "utility/ranges.hpp"
#include "utility/variant.hpp"

#include "ful/convert.hpp"
#include "ful/cstrext.hpp"
#include "ful/heap.hpp"
#include "ful/point.hpp"
#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"
#include "ful/view.hpp"

#include <atomic>
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

			extern core::container::PageQueue<utility::heap_storage<int, int, engine::Token, engine::Command>> queue_select;

			extern std::atomic<int> entitytoggle;

			extern engine::application::window * window;
			extern void (* callback_select)(engine::Token entity, engine::Command command, utility::any && data);
		}
	}
}

static_hashes("box", "cuboid", "dude", "my_png", "photo", "_entity_");

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

	struct ShaderManager
	{
		struct Data
		{
			GLint program;
			GLint vertex;
			GLint fragment;

			utility::heap_vector<GLint, ful::heap_string_utf8> uniforms;
		};

		utility::heap_vector<engine::Token, Data> shaders;

		const Data * end() const { return shaders.end().second; }

		const Data * find(engine::Asset asset) const
		{
			return ext::find_if(shaders, fun::first == asset).second;
		}

		bool create(engine::Token asset, engine::graphics::data::ShaderData && shader_data)
		{
			if (!debug_verify(!ext::contains_if(shaders, fun::first == asset), "shader ", asset, " has already been created"))
				return false;

			if (!debug_verify(shaders.try_emplace_back()))
				return false;

			auto && shader = ext::back(shaders);
			shader.first = asset;

			shader.second.vertex = glCreateShader(GL_VERTEX_SHADER);
			const char * vs_source = shader_data.vertex_source.data();
			glShaderSource(shader.second.vertex, 1, &vs_source, nullptr);
			glCompileShader(shader.second.vertex);
			GLint vs_compile_status;
			glGetShaderiv(shader.second.vertex, GL_COMPILE_STATUS, &vs_compile_status);
			if (!vs_compile_status)
			{
				char buffer[1000];
				int length;
				glGetShaderInfoLog(shader.second.vertex, 1000, &length, buffer);

				ext::pop_back(shaders);

				return debug_fail("vertex shader entity failed to compile with: ", buffer);
			}

			shader.second.fragment = glCreateShader(GL_FRAGMENT_SHADER);
			const char * fs_source = shader_data.fragment_source.data();
			glShaderSource(shader.second.fragment, 1, &fs_source, nullptr);
			glCompileShader(shader.second.fragment);
			GLint fs_compile_status;
			glGetShaderiv(shader.second.fragment, GL_COMPILE_STATUS, &fs_compile_status);
			if (!fs_compile_status)
			{
				char buffer[1000];
				int length;
				glGetShaderInfoLog(shader.second.fragment, 1000, &length, buffer);

				glDeleteShader(shader.second.vertex);
				ext::pop_back(shaders);

				return debug_fail("fragment shader entity failed to compile with: ", buffer);
			}

			shader.second.program = glCreateProgram();
			glAttachShader(shader.second.program, shader.second.vertex);
			glAttachShader(shader.second.program, shader.second.fragment);
			for (const auto & input : shader_data.inputs)
			{
				glBindAttribLocation(shader.second.program, input.value, input.name.data());
			}
			for (const auto & output : shader_data.outputs)
			{
				glBindFragDataLocation(shader.second.program, output.value, output.name.data());
			}
			glLinkProgram(shader.second.program);
			GLint p_link_status;
			glGetProgramiv(shader.second.program, GL_LINK_STATUS, &p_link_status);
			if (!p_link_status)
			{
				char buffer[1000];
				int length;
				glGetProgramInfoLog(shader.second.program, 1000, &length, buffer);

				glDeleteShader(shader.second.fragment);
				glDeleteShader(shader.second.vertex);
				ext::pop_back(shaders);

				return debug_fail("program entity failed to link with: ", buffer);
			}

			if (!debug_assert(glGetError() == GL_NO_ERROR))
			{
				glDeleteProgram(shader.second.program);
				glDeleteShader(shader.second.fragment);
				glDeleteShader(shader.second.vertex);
				ext::pop_back(shaders);

				return false;
			}

			auto fragment_parser = rex::parse(shader_data.fragment_source);
			while (true)
			{
				const auto uniform_declaration = fragment_parser.find((rex::str(ful::cstr_utf8("uniform")) >> +rex::blank) | (rex::ch('/') >> ((rex::ch('/') >> *!rex::newline >> rex::newline) | (rex::ch('*') >> *!rex::str(ful::cstr_utf8("*/")) >> rex::str(ful::cstr_utf8("*/"))))));
				if (uniform_declaration.first == uniform_declaration.second)
					break;

				fragment_parser.seek(uniform_declaration.second);
				if (*uniform_declaration.first == '/')
					continue;

				const auto uniform_type = fragment_parser.match(+rex::word);
				if (!uniform_type.first)
					continue;

				fragment_parser.seek(uniform_type.second);
				const auto uniform_type_space_name = fragment_parser.match(+rex::blank);
				if (!uniform_type_space_name.first)
					continue;

				fragment_parser.seek(uniform_type_space_name.second);
				const auto uniform_name = fragment_parser.match(+rex::word);
				if (!uniform_name.first)
					continue;

				fragment_parser.seek(uniform_name.second);
				const auto uniform_semicolon = fragment_parser.match(*rex::blank >> rex::ch(';'));
				if (!uniform_semicolon.first)
					continue;

				const auto type = ful::view_utf8(uniform_declaration.second, uniform_type.second);
				const auto name = ful::view_utf8(uniform_type_space_name.second, uniform_name.second);

				if (type == u8"sampler2D")
				{
					if (debug_verify(shader.second.uniforms.try_emplace_back()))
					{
						ext::back(shader.second.uniforms).first = GL_TEXTURE_2D;
						if (!debug_verify(ful::copy(name, ext::back(shader.second.uniforms).second)))
						{
							ext::pop_back(shader.second.uniforms);
						}
					}
				}
			}

			return true;
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
	GLenum glType(utility::type_id_t type)
	{
		switch (type)
		{
		case utility::type_id<uint8_t>(): return GL_UNSIGNED_BYTE;
		case utility::type_id<uint16_t>(): return GL_UNSIGNED_SHORT;
		case utility::type_id<uint32_t>(): return GL_UNSIGNED_INT;
		case utility::type_id<int8_t>(): return GL_BYTE;
		case utility::type_id<int16_t>(): return GL_SHORT;
		case utility::type_id<int32_t>(): return GL_INT;
		case utility::type_id<float>(): return GL_FLOAT;
		case utility::type_id<double>(): return GL_DOUBLE;
		default: debug_unreachable(type, " not supported");
		}
	}

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
		engine::Token,
		utility::static_storage_traits<23>,
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
			std::vector<ful::point_utf> allowed_unicodes;
			std::vector<SymbolData> symbol_data;

			int symbol_width;
			int symbol_height;
			int texture_width; // ?
			int texture_height; // ?

			ful::heap_string_utf8 name;
			int size;
		};

		engine::Token assets[10];
		FontInfo infos[10];
		int count = 0;

		std::ptrdiff_t find(engine::Token asset) const
		{
			return std::find(assets, assets + count, asset) - assets;
		}

		bool has(engine::Token asset) const
		{
			return find(asset) < count;
		}

		// todo boundary_symbol
		void compile(engine::Token asset, ful::view_utf8 text, core::container::Buffer & vertices, core::container::Buffer & texcoords)
		{
			const auto index = find(asset);
			debug_assert(index < count, "font asset ", asset, " does not exist");

			ful::heap_string_utf32 text32;
			if (!debug_verify(ful::convert(text, text32)))
				return;

			const std::ptrdiff_t len = text32.size();
			if (!debug_verify(vertices.reshape<float>(4 * len * 2)))
				return;

			if (!debug_verify(texcoords.reshape<float>(4 * len * 2)))
				return;

			const FontInfo & info = infos[index];

			const int slots_in_width = info.texture_width / info.symbol_width;

			float * vertex_data = vertices.data_as<float>();
			float * texcoord_data = texcoords.data_as<float>();

			int x = 0;
			int y = 0;
			int i = 0;
			for (auto cp : text32)
			{
				// the null glyph is stored at the end
				const auto maybe = std::lower_bound(info.allowed_unicodes.begin(), info.allowed_unicodes.end(), ful::point_utf(cp));
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

		void create(ful::heap_string_utf8 && name, std::vector<ful::point_utf> && allowed_unicodes, std::vector<SymbolData> && symbol_data, int symbol_width, int symbol_height, int texture_width, int texture_height)
		{
			const engine::Asset asset(name);
			const auto index = find(engine::Token(asset));
			debug_assert(index >= count, "font asset ", asset, " already exists");

			assets[index] = engine::Token(asset);

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
			const auto index = find(engine::Token(asset));
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

	struct ColorClass
	{
		engine::graphics::opengl::Color4ub diffuse;
		engine::Token shader;

		explicit ColorClass(uint32_t diffuse, engine::Token shader)
			: diffuse(diffuse >> 0 & 0x000000ff,
				diffuse >> 8 & 0x000000ff,
				diffuse >> 16 & 0x000000ff,
				diffuse >> 24 & 0x000000ff)
			, shader(shader)
		{}
	};

	struct Shader
	{
		GLint program;
		GLint vertex;
		GLint fragment;

		utility::heap_vector<GLint, ful::heap_string_utf8> uniforms;
	};

	struct ShaderClass
	{
		engine::Token shader;

		explicit ShaderClass(engine::Token shader)
			: shader(shader)
		{}
	};

	struct Texture
	{
		GLuint id;

		~Texture()
		{
			if (id != GLuint(-1))
			{
				glDeleteTextures(1, &id);
			}
		}

		Texture(Texture && other)
			: id(std::exchange(other.id, GLuint(-1)))
		{}

		explicit Texture(core::graphics::Image && image)
		{
			glEnable(GL_TEXTURE_2D);

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			switch (image.color())
			{
			case core::graphics::ColorType::R:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, image.width(), image.height(), 0, GL_RED, glType(image.pixels().value_type()), image.data());
				break;
			case core::graphics::ColorType::RGB:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, glType(image.pixels().value_type()), image.data());
				break;
			case core::graphics::ColorType::RGBA:
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, glType(image.pixels().value_type()), image.data());
				break;
			default:
				debug_fail("color type not supported");
			}

			glDisable(GL_TEXTURE_2D);

			debug_assert(glGetError() == GL_NO_ERROR);
		}

		Texture & operator = (Texture && other)
		{
			if (id != GLuint(-1))
			{
				glDeleteTextures(1, &id);
			}

			id = std::exchange(other.id, GLuint(-1));

			return *this;
		}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<801>,
		utility::static_storage<100, mesh_t>,
		utility::static_storage<100, ColorClass>,
		utility::static_storage<100, Shader>,
		utility::static_storage<100, ShaderClass>,
		utility::static_storage<100, Texture>
	>
	resources;

	bool create_shader(engine::Token asset, engine::graphics::data::ShaderData && shader_data)
	{
		Shader * const shader = resources.emplace<Shader>(asset);
		if (!debug_verify(shader))
			return false;

		shader->vertex = glCreateShader(GL_VERTEX_SHADER);
		const char * vs_source = shader_data.vertex_source.data();
		glShaderSource(shader->vertex, 1, &vs_source, nullptr);
		glCompileShader(shader->vertex);
		GLint vs_compile_status;
		glGetShaderiv(shader->vertex, GL_COMPILE_STATUS, &vs_compile_status);
		if (!vs_compile_status)
		{
			char buffer[1000];
			int length;
			glGetShaderInfoLog(shader->vertex, 1000, &length, buffer);

			resources.erase(find(resources, asset));

			return debug_fail("vertex shader entity failed to compile with: ", buffer);
		}

		shader->fragment = glCreateShader(GL_FRAGMENT_SHADER);
		const char * fs_source = shader_data.fragment_source.data();
		glShaderSource(shader->fragment, 1, &fs_source, nullptr);
		glCompileShader(shader->fragment);
		GLint fs_compile_status;
		glGetShaderiv(shader->fragment, GL_COMPILE_STATUS, &fs_compile_status);
		if (!fs_compile_status)
		{
			char buffer[1000];
			int length;
			glGetShaderInfoLog(shader->fragment, 1000, &length, buffer);

			glDeleteShader(shader->vertex);
			resources.erase(find(resources, asset));

			return debug_fail("fragment shader entity failed to compile with: ", buffer);
		}

		shader->program = glCreateProgram();
		glAttachShader(shader->program, shader->vertex);
		glAttachShader(shader->program, shader->fragment);
		for (const auto & input : shader_data.inputs)
		{
			glBindAttribLocation(shader->program, input.value, input.name.data());
		}
		for (const auto & output : shader_data.outputs)
		{
			glBindFragDataLocation(shader->program, output.value, output.name.data());
		}
		glLinkProgram(shader->program);
		GLint p_link_status;
		glGetProgramiv(shader->program, GL_LINK_STATUS, &p_link_status);
		if (!p_link_status)
		{
			char buffer[1000];
			int length;
			glGetProgramInfoLog(shader->program, 1000, &length, buffer);

			glDeleteShader(shader->fragment);
			glDeleteShader(shader->vertex);
			resources.erase(find(resources, asset));

			return debug_fail("program entity failed to link with: ", buffer);
		}

		if (!debug_assert(glGetError() == GL_NO_ERROR))
		{
			glDeleteProgram(shader->program);
			glDeleteShader(shader->fragment);
			glDeleteShader(shader->vertex);
			resources.erase(find(resources, asset));

			return false;
		}

		auto fragment_parser = rex::parse(shader_data.fragment_source);
		while (true)
		{
			const auto uniform_declaration = fragment_parser.find((rex::str(ful::cstr_utf8("uniform")) >> +rex::blank) | (rex::ch('/') >> ((rex::ch('/') >> *!rex::newline >> rex::newline) | (rex::ch('*') >> *!rex::str(ful::cstr_utf8("*/")) >> rex::str(ful::cstr_utf8("*/"))))));
			if (uniform_declaration.first == uniform_declaration.second)
				break;

			fragment_parser.seek(uniform_declaration.second);
			if (*uniform_declaration.first == '/')
				continue;

			const auto uniform_type = fragment_parser.match(+rex::word);
			if (!uniform_type.first)
				continue;

			fragment_parser.seek(uniform_type.second);
			const auto uniform_type_space_name = fragment_parser.match(+rex::blank);
			if (!uniform_type_space_name.first)
				continue;

			fragment_parser.seek(uniform_type_space_name.second);
			const auto uniform_name = fragment_parser.match(+rex::word);
			if (!uniform_name.first)
				continue;

			fragment_parser.seek(uniform_name.second);
			const auto uniform_semicolon = fragment_parser.match(*rex::blank >> rex::ch(';'));
			if (!uniform_semicolon.first)
				continue;

			const auto type = ful::view_utf8(uniform_declaration.second, uniform_type.second);
			const auto name = ful::view_utf8(uniform_type_space_name.second, uniform_name.second);

			if (type == u8"sampler2D")
			{
				if (debug_verify(shader->uniforms.try_emplace_back()))
				{
					ext::back(shader->uniforms).first = GL_TEXTURE_2D;
					if (!debug_verify(ful::copy(name, ext::back(shader->uniforms).second)))
					{
						ext::pop_back(shader->uniforms);
					}
				}
			}
		}

		return true;
	}

	void destroy_shader(Shader & shader)
	{
		glDeleteProgram(shader.program);
		glDeleteShader(shader.fragment);
		glDeleteShader(shader.vertex);
	}

	struct ShaderMaterial
	{
		engine::graphics::opengl::Color4ub diffuse;
		utility::heap_vector<engine::Token> textures;

		engine::Token materialclass;

		explicit ShaderMaterial(uint32_t diffuse, utility::heap_vector<engine::Token> && textures, engine::Token materialclass)
			: diffuse(diffuse >> 0 & 0x000000ff,
				diffuse >> 8 & 0x000000ff,
				diffuse >> 16 & 0x000000ff,
				diffuse >> 24 & 0x000000ff)
			, textures(std::move(textures))
			, materialclass(materialclass)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<201>,
		utility::static_storage<100, ShaderMaterial>
	>
	materials;


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
		{
			debug_verify(copy(mesh.vertices, vertices)); // todo
		}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<1601>,
		utility::static_storage<600, object_modelview>,
		utility::static_storage<200, object_modelview_vertices>
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
		object_modelview_vertices * object;

		Character(
			const mesh_t & mesh,
			object_modelview_vertices & object)
			: mesh(&mesh)
			, object(&object)
		{}
	};

	struct MeshObject
	{
		const object_modelview * object;
		const mesh_t * mesh;
		engine::Token material;
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<Character>,
		utility::heap_storage<MeshObject>
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
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<selectable_character_t>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		void operator () (engine::Token entity, Character & x)
		{
			debug_verify(selectable_components.emplace<selectable_character_t>(entity, x.mesh, x.object, color));
		}

		template <typename T>
		void operator () (engine::Token /*entity*/, T &)
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
			for (std::ptrdiff_t i : ranges::index_sequence(object->vertices.size() / 3))
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
		engine::Token,
		utility::heap_storage_traits,
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
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<highlighted_t>,
		utility::heap_storage<selected_t>
	>
	selected_components;
}

namespace
{
	using namespace engine::graphics::detail;

	FontManager font_manager;

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
					debug_verify(displays.emplace<display_t>(
						             x.asset,
						             x.display.viewport.x, x.display.viewport.y, x.display.viewport.width, x.display.viewport.height,
						             x.display.camera_3d.projection, x.display.camera_3d.frame, x.display.camera_3d.view, x.display.camera_3d.inv_projection, x.display.camera_3d.inv_frame, x.display.camera_3d.inv_view,
						             x.display.camera_2d.projection, x.display.camera_2d.view));
					should_maybe_resize_framebuffer = true;
				}

				void operator () (MessageRemoveDisplay && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.erase(display_it);
						should_maybe_resize_framebuffer = true;
					}
				}

				void operator () (MessageUpdateDisplayCamera2D && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_camera_2d{std::move(x.camera_2d)});
					}
				}

				void operator () (MessageUpdateDisplayCamera3D && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_camera_3d{std::move(x.camera_3d)});
					}
				}

				void operator () (MessageUpdateDisplayViewport && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_viewport{std::move(x.viewport)});
						should_maybe_resize_framebuffer = true;
					}
				}

				void operator () (MessageRegisterCharacter && x)
				{
					debug_verify(resources.emplace<mesh_t>(x.asset, std::move(x.mesh)));
				}

				void operator () (MessageRegisterMaterial && x)
				{
					if (!debug_verify(x.material.shader))
						return; // error

					if (x.material.diffuse)
					{
						debug_verify(resources.emplace<ColorClass>(x.asset, x.material.diffuse.value(), engine::Token(x.material.shader.value())));
					}
					else
					{
						debug_verify(resources.emplace<ShaderClass>(x.asset, engine::Token(x.material.shader.value())));
					}
				}

				void operator () (MessageRegisterMesh && x)
				{
					debug_verify(resources.emplace<mesh_t>(x.asset, std::move(x.mesh)));
				}

				void operator () (MessageRegisterShader && x)
				{
					if (!create_shader(x.asset, std::move(x.data)))
						return; // error
				}

				void operator () (MessageRegisterTexture && x)
				{
					if (!debug_verify(resources.emplace<Texture>(x.asset, std::move(x.image))))
						return; // error
				}

				void operator () (MessageUnregister && x)
				{
					const auto it = find(resources, x.asset);
					if (!debug_verify(it != resources.end()))
						return; // error

					resources.call(it,
						[](mesh_t &){},
						[](ColorClass &){},
						[](Shader & y){ destroy_shader(y); },
						[](ShaderClass &){},
						[](Texture &){});

					resources.erase(it);
				}

				void operator () (MessageCreateMaterialInstance && x)
				{
					const auto material_it = find(materials, x.entity);
					if (material_it != materials.end())
					{
						if (!debug_assert(materials.get_key(material_it) < x.entity, "trying to add an older version object"))
							return; // error

						materials.erase(material_it);
					}

					utility::heap_vector<engine::Token> textures;
					if (!debug_verify(textures.try_reserve(x.data.textures.size())))
						return; // error

					for (const auto & texture : x.data.textures)
					{
						textures.try_emplace_back(utility::no_failure, texture.texture);
					}

					debug_verify(materials.emplace<ShaderMaterial>(x.entity, x.data.diffuse, std::move(textures), x.data.materialclass));
				}

				void operator () (MessageDestroy && x)
				{
					auto material_it = find(materials, x.entity);
					if (material_it != materials.end())
					{
						materials.erase(material_it);
					}
				}

				void operator () (MessageAddMeshObject && x)
				{
					const auto object_it = find(objects, x.entity);
					if (object_it != objects.end())
					{
						if (!debug_assert(objects.get_key(object_it) < x.entity, "trying to add an older version object"))
							return; // error

						objects.erase(object_it);
					}
					auto * const object = objects.emplace<object_modelview>(x.entity, std::move(x.object.matrix));

					const auto mesh_it = find(resources, x.object.mesh);
					if (!debug_verify(mesh_it != resources.end(), "missing mesh"))
						return; // error

					if (!debug_verify(resources.contains<mesh_t>(mesh_it), "invalid mesh"))
						return; // error

					mesh_t * const mesh = resources.get<mesh_t>(mesh_it);

					const auto component_it = find(components, x.entity);
					if (component_it != components.end())
					{
						if (!debug_assert(components.get_key(component_it) < x.entity, "trying to add an older version mesh"))
							return; // error

						components.erase(component_it);
					}
					debug_verify(components.emplace<MeshObject>(x.entity, object, mesh, x.object.material));
				}

				void operator () (MessageMakeObstruction && x)
				{
					const engine::graphics::opengl::Color4ub color = {0, 0, 0, 0};

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.call(selectable_it, set_selectable_color{color});
					}
					else
					{
						const auto component_it = find(components, x.entity);
						if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
							return; // error

						components.call(component_it, add_selectable_color{color});
					}
				}

				void operator () (MessageMakeSelectable && x)
				{
					const engine::graphics::opengl::Color4ub color = {(x.entity & 0x000000ff) >> 0,
					                                                  (x.entity & 0x0000ff00) >> 8,
					                                                  (x.entity & 0x00ff0000) >> 16,
					                                                  (x.entity & 0xff000000) >> 24};

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.call(selectable_it, set_selectable_color{color});
					}
					else
					{
						const auto component_it = find(components, x.entity);
						if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
							return; // error

						components.call(component_it, add_selectable_color{color});
					}
				}

				void operator () (MessageMakeTransparent && x)
				{
					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.erase(selectable_it);
					}
				}

				void operator () (MessageMakeClearSelection &&)
				{
					debug_fail(); // not implemented yet
				}

				void operator () (MessageMakeDehighlighted && x)
				{
					const auto selected_it = find(selected_components, x.entity);
					if (selected_it != selected_components.end())
					{
						if (selected_components.contains<highlighted_t>(selected_it))
						{
							selected_components.erase<highlighted_t>(selected_it);
						}
					}
				}

				void operator () (MessageMakeDeselect && x)
				{
					const auto selected_it = find(selected_components, x.entity);
					if (selected_it != selected_components.end())
					{
						if (selected_components.contains<selected_t>(selected_it))
						{
							selected_components.erase<selected_t>(selected_it);
						}
					}
				}

				void operator () (MessageMakeHighlighted && x)
				{
					debug_verify(selected_components.emplace<highlighted_t>(x.entity));
				}

				void operator () (MessageMakeSelect && x)
				{
					debug_verify(selected_components.emplace<selected_t>(x.entity));
				}

				void operator () (MessageRemove && x)
				{
					const auto component_it = find(components, x.entity);
					if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
						return; // error

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.erase(selectable_it);
					}

					const auto updateable_it = find(updateable_components, x.entity);
					if (updateable_it != updateable_components.end())
					{
						updateable_components.erase(updateable_it);
					}

					components.erase(component_it);

					const auto object_it = find(objects, x.entity);
					if (debug_assert(object_it != objects.end()))
					{
						objects.erase(object_it);
					}
				}

				void operator () (MessageUpdateCharacterSkinning && x)
				{
					const auto updateable_it = find(updateable_components, x.entity);
					if (debug_assert(updateable_it != updateable_components.end()))
					{
						updateable_components.call(updateable_it, update_matrixpallet{std::move(x.character_skinning)});
					}
				}

				void operator () (MessageUpdateModelviewMatrix && x)
				{
					const auto object_it = find(objects, x.entity);
					if (debug_assert(object_it != objects.end()))
					{
						objects.call(object_it, update_modelview{std::move(x.modelview_matrix)});
					}
				}
			};
			visit(ProcessMessage{should_maybe_resize_framebuffer}, std::move(message));
		}
		if (should_maybe_resize_framebuffer)
		{
			maybe_resize_framebuffer();
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

	engine::Token get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = framebuffer_height - 1 - sy;

		if (x >= 0 &&
		    y >= 0 &&
		    x < 0 + framebuffer_width &&
		    y < 0 + framebuffer_height)
		{
			const unsigned int color = entitypixels[x + y * framebuffer_width];
			return engine::Token(
				(color & 0xff000000) >> 24 |
				(color & 0x00ff0000) >> 8 |
				(color & 0x0000ff00) << 8 |
				(color & 0x000000ff) << 24);
		}
		return engine::Token{};
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

	constexpr const auto entity_shader_asset = engine::Hash("_entity_");

	void initialize_builtin_shaders()
	{
		engine::graphics::data::ShaderData entity_shader_data;
		entity_shader_data.inputs.emplace_back();
		entity_shader_data.inputs.back().value = 4;
		if (!debug_verify(ful::assign(entity_shader_data.inputs.back().name, ful::cstr_utf8("in_vertex"))))
			return; // error
		entity_shader_data.inputs.emplace_back();
		entity_shader_data.inputs.back().value = 5;
		if (!debug_verify(ful::assign(entity_shader_data.inputs.back().name, ful::cstr_utf8("in_color"))))
			return; // error
		if (!debug_verify(ful::assign(entity_shader_data.vertex_source, ful::cstr_utf8(R"###(
#version 130

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

in vec3 in_vertex;
in vec4 in_color;

out vec4 color;

void main()
{
	gl_Position = projection_matrix * modelview_matrix * vec4(in_vertex.xyz, 1.f);
	color = in_color;
}
)###"))))
			return; // error
		entity_shader_data.outputs.emplace_back();
		entity_shader_data.outputs.back().value = 0;
		if (!debug_verify(ful::assign(entity_shader_data.outputs.back().name, ful::cstr_utf8("out_framebuffer"))))
			return; // error
		entity_shader_data.outputs.emplace_back();
		entity_shader_data.outputs.back().value = 1;
		if (!debug_verify(ful::assign(entity_shader_data.outputs.back().name, ful::cstr_utf8("out_entitytex"))))
			return; // error
		if (!debug_verify(ful::assign(entity_shader_data.fragment_source, ful::cstr_utf8(R"###(
#version 130

in vec4 color;

out vec4 out_framebuffer;
out vec4 out_entitytex;

void main()
{
	out_framebuffer = color;
	out_entitytex = color;
}
)###"))))
			return; // error
		debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterShader>, engine::Token(entity_shader_asset), std::move(entity_shader_data)));
	}

	void render_setup()
	{
		debug_printline(engine::graphics_channel, "render_callback starting");
		make_current(*engine::graphics::detail::window);

		debug_printline(engine::graphics_channel, "glGetString GL_VENDOR: ", ful::make_cstr_utf8(glGetString(GL_VENDOR)));
		debug_printline(engine::graphics_channel, "glGetString GL_RENDERER: ", ful::make_cstr_utf8(glGetString(GL_RENDERER)));
		debug_printline(engine::graphics_channel, "glGetString GL_VERSION: ", ful::make_cstr_utf8(glGetString(GL_VERSION)));

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

		initialize_builtin_shaders();

		debug_assert(glGetError() == GL_NO_ERROR);
	}

	void render_update()
	{
		profile_scope("renderer update (opengl 3.0)");

		// poll events
		poll_queues();
		// update components
		for (auto & component : updateable_components.get<updateable_character_t>())
		{
			component.update();
		}

		if (framebuffer_width == 0 || framebuffer_height == 0)
			return;

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
		const auto entity_shader_it = find(resources, entity_shader_asset);
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4127 )
// C4127 - conditional expression is constant
#endif
		if (debug_assert(entity_shader_it != resources.end()) && debug_assert(resources.contains<Shader>(entity_shader_it)))
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
		{
			const Shader * const entity_shader = resources.get<Shader>(entity_shader_it);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, framebuffer_width, framebuffer_height);
			glClearColor(0.f, 0.f, 0.f, 0.f); // null entity
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			debug_assert(glGetError() == GL_NO_ERROR);

			glDisable(GL_LIGHTING);
			glEnable(GL_DEPTH_TEST);

			glUseProgram(entity_shader->program);

			for (const auto & display : displays.get<display_t>())
			{
				glViewport(display.x, display.y, display.width, display.height);

				// setup 3D
				glMatrixMode(GL_PROJECTION);
				glLoadMatrix(display.projection_3d);
				glUniform(entity_shader->program, "projection_matrix", display.projection_3d);
				glMatrixMode(GL_MODELVIEW);
				modelview_matrix.load(display.view_3d);

				debug_assert(glGetError() == GL_NO_ERROR);

				for (const auto & component : selectable_components.get<selectable_character_t>())
				{
					modelview_matrix.push();
					modelview_matrix.mult(component.object->modelview);
					glUniform(entity_shader->program, "modelview_matrix", modelview_matrix.top());

					const auto vertex_location = 4;
					glEnableVertexAttribArray(vertex_location);
					glVertexAttribPointer(
						vertex_location,
						3, // todo support 2d coordinates?
						GL_FLOAT,
						GL_FALSE,
						0,
						component.object->vertices.data());

					const auto color_location = 5;
					glVertexAttrib4f(color_location, component.selectable_color[0] / 255.f, component.selectable_color[1] / 255.f, component.selectable_color[2] / 255.f, component.selectable_color[3] / 255.f);

					glDrawElements(
						GL_TRIANGLES,
						debug_cast<GLsizei>(component.mesh->triangles.size()),
						GL_UNSIGNED_SHORT, // TODO
						component.mesh->triangles.data());

					glDisableVertexAttribArray(vertex_location);

					debug_assert(glGetError() == GL_NO_ERROR);

					modelview_matrix.pop();
				}

				// setup 2D
				glMatrixMode(GL_PROJECTION);
				glLoadMatrix(display.projection_2d);
				glUniform(entity_shader->program, "projection_matrix", display.projection_2d);
				glMatrixMode(GL_MODELVIEW);
				modelview_matrix.load(display.view_2d);
			}

			glUseProgram(0);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

		glReadPixels(0, 0, framebuffer_width, framebuffer_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, entitypixels.data());

		// select
		{
			std::tuple<int, int, engine::Token, engine::Command> select_args;
			while (queue_select.try_pop(select_args))
			{
				engine::graphics::data::SelectData select_data = {get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)), {std::get<0>(select_args), std::get<1>(select_args)}};
				callback_select(std::get<2>(select_args), std::get<3>(select_args), utility::any(std::move(select_data)));
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

		for (auto & component : components.get<MeshObject>())
		{
			engine::graphics::opengl::Color4ub color(0, 0, 0, 0);
			utility::static_vector<80, GLint> textures;
			engine::Token shader_asset{};

			const auto material_it = find(materials, component.material);
			if (material_it != materials.end())
			{
				if (auto * const material = materials.get<ShaderMaterial>(material_it))
				{
					color = material->diffuse;
					debug_assert(textures.try_reserve(material->textures.size()));
					for (const auto & texture : material->textures)
					{
						const auto texture_it = find(resources, texture);
						if (texture_it != resources.end())
						{
							if (const Texture * texture_ptr = resources.get<Texture>(texture_it))
							{
								textures.try_emplace_back(utility::no_failure, texture_ptr->id);
							}
						}
					}

					const auto resource_it = find(resources, material->materialclass);
					if (resource_it != resources.end())
					{
						if (auto * const color_class = resources.get<ColorClass>(resource_it))
						{
							color = color_class->diffuse;
							shader_asset = color_class->shader;
						}
						else if (auto * const shader_class = resources.get<ShaderClass>(resource_it))
						{
							shader_asset = shader_class->shader;
						}
					}
				}
			}

			const auto shader_it = find(resources, shader_asset);
			if (!(shader_it != resources.end() && resources.contains<Shader>(shader_it)))
				continue; // todo not ready (yet?)

			const Shader * const shader = resources.get<Shader>(shader_it);

			glUseProgram(shader->program);
			glUniform(shader->program, "projection_matrix", display.projection_3d);
			{
				static int frame_count = 0;
				frame_count++;

				glUniform(shader->program, "time", static_cast<float>(frame_count) / 50.f);
			}
			glUniform(shader->program, "dimensions", static_cast<float>(framebuffer_width), static_cast<float>(framebuffer_height));

			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			modelview_matrix.mult(component.mesh->modelview);
			glUniform(shader->program, "modelview_matrix", modelview_matrix.top());

			const auto entity = components.get_key(component);
			const auto selected_it = find(selected_components, entity);
			const bool is_highlighted = selected_it != selected_components.end() && selected_components.contains<highlighted_t>(selected_it);
			const bool is_selected = selected_it != selected_components.end() && selected_components.contains<selected_t>(selected_it);
			const bool is_interactible = find(selectable_components, entity) != selectable_components.end();

			const auto status_flags_location = 4;
			glVertexAttrib4f(status_flags_location, static_cast<float>(is_highlighted), static_cast<float>(is_selected), 0.f, static_cast<float>(is_interactible));

			GLint texture_slot = 0;
			const auto texture_it = textures.begin();
			for (const auto & uniform : shader->uniforms)
			{
				switch (uniform.first)
				{
				case GL_TEXTURE_2D:
					glActiveTexture(GL_TEXTURE0 + texture_slot);
					if (uniform.second == u8"_Entity")
					{
						glBindTexture(GL_TEXTURE_2D, entitytexture);
					}
					else if (debug_verify(texture_it != textures.end()))
					{
						glBindTexture(GL_TEXTURE_2D, *texture_it);
					}
					glUniform(shader->program, uniform.second.data(), texture_slot);
					texture_slot++;
					break;
				}
			}

			const auto vertex_location = 5;
			const auto normal_location = 6;
			const auto texcoord_location = 7;
			if (!empty(component.mesh->vertices))
			{
				glEnableVertexAttribArray(vertex_location);
			}
			if (!empty(component.mesh->normals))
			{
				glEnableVertexAttribArray(normal_location);
			}
			if (!empty(component.mesh->coords))
			{
				glEnableVertexAttribArray(texcoord_location);
			}

			const auto color_location = 8;
			glVertexAttrib4f(color_location, color[0] / 255.f, color[1] / 255.f, color[2] / 255.f, color[3] / 255.f);

			if (!empty(component.mesh->vertices))
			{
				glVertexAttribPointer(
					vertex_location,
					3, // todo support 2d coordinates?
					glType(component.mesh->vertices.value_type()),
					GL_FALSE,
					0,
					component.mesh->vertices.data());
			}
			if (!empty(component.mesh->normals))
			{
				glVertexAttribPointer(
					normal_location,
					3, // todo support 2d coordinates?
					glType(component.mesh->normals.value_type()),
					GL_FALSE,
					0,
					component.mesh->normals.data());
			}
			if (!empty(component.mesh->coords))
			{
				glVertexAttribPointer(
					texcoord_location,
					2,
					glType(component.mesh->coords.value_type()),
					GL_FALSE,
					0,
					component.mesh->coords.data());
			}
			glDrawElements(
				GL_TRIANGLES,
				debug_cast<GLsizei>(component.mesh->triangles.size()),
				glType(component.mesh->triangles.value_type()),
				component.mesh->triangles.data());

			if (!empty(component.mesh->coords))
			{
				glDisableVertexAttribArray(texcoord_location);
			}
			if (!empty(component.mesh->normals))
			{
				glDisableVertexAttribArray(normal_location);
			}
			if (!empty(component.mesh->vertices))
			{
				glDisableVertexAttribArray(vertex_location);
			}

			modelview_matrix.pop();

			glUseProgram(0);

			debug_assert(glGetError() == GL_NO_ERROR);
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
		debug_assert(glGetError() == GL_NO_ERROR);

		// 2d
		// ...

		// clear depth to make GUI show over all prev. rendering
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);
		debug_assert(glGetError() == GL_NO_ERROR);
		}

		// entity buffers
		if (entitytoggle.load(std::memory_order_relaxed))
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

			glBlitFramebuffer(0, 0, framebuffer_width, framebuffer_height, 0, 0, framebuffer_width, framebuffer_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		debug_assert(glGetError() == GL_NO_ERROR);

		// swap buffers
		swap_buffers(*engine::graphics::detail::window);
		debug_assert(glGetError() == GL_NO_ERROR);
	}

	void render_teardown()
	{
		debug_printline(engine::graphics_channel, "render_callback stopping");

		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);

		engine::Token resources_not_unregistered[resources.max_size()];
		const auto resource_count = resources.get_all_keys(resources_not_unregistered, resources.max_size());
		debug_printline(engine::asset_channel, resource_count, " resources not unregistered:");
		for (auto i : ranges::index_sequence(resource_count))
		{
			debug_printline(engine::asset_channel, resources_not_unregistered[i]);
			static_cast<void>(i);
		}

		engine::Token materials_not_destroyed[materials.max_size()];
		const auto material_count = materials.get_all_keys(materials_not_destroyed, materials.max_size());
		debug_printline(engine::asset_channel, material_count, " materials not destroyed:");
		for (auto i : ranges::index_sequence(material_count))
		{
			debug_printline(engine::asset_channel, materials_not_destroyed[i]);
			static_cast<void>(i);
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

#endif
