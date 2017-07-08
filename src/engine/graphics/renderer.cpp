
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/color.hpp>
#include <core/async/Thread.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/container/Stack.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>
#include <core/sync/Event.hpp>

#include <engine/Asset.hpp>
#include <engine/Command.hpp>
#include <engine/debug.hpp>

#include <atomic>
#include <fstream>
#include <utility>

using namespace engine::graphics::opengl;

namespace engine
{
namespace application
{
namespace window
{
	extern void make_current();
	extern void swap_buffers();
}
}
}

namespace gameplay
{
namespace gamestate
{
	extern void post_command(engine::Entity entity, engine::Command command, engine::Entity arg1);
}
}

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
}

namespace
{
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

		texture_t(core::graphics::Image && image)
		{
			glEnable(GL_TEXTURE_2D);

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_LINEAR*/GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_NEAREST);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			switch (image.color())
			{
			case core::graphics::ImageColor::RGB:
				glTexImage2D(GL_TEXTURE_2D, 0, 3, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
				break;
			case core::graphics::ImageColor::RGBA:
				glTexImage2D(GL_TEXTURE_2D, 0, 4, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
				break;
			default:
				debug_unreachable();
			}

			glDisable(GL_TEXTURE_2D);
		}

		void enable() const
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, id);
		}
		void disable() const
		{
			glDisable(GL_TEXTURE_2D);
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

	// the vertices of cuboid_c are numbered as followed:
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
	struct cuboid_c
	{
		core::maths::Matrix4x4f modelview;
		std::array<float, 3 * 24> vertices;
		engine::graphics::opengl::Color4ub color;
		bool is_transparent; // ugh!!

		static const std::array<uint16_t, 3 * 12> triangles;
		static const std::array<float, 3 * 24> normals;

		cuboid_c(engine::graphics::data::CuboidC && data) :
			modelview(std::move(data.modelview)),
			color((data.color & 0x000000ff) >>  0,
			      (data.color & 0x0000ff00) >>  8,
			      (data.color & 0x00ff0000) >> 16,
			      (data.color & 0xff000000) >> 24),
			is_transparent{bool((data.color & 0xff000000) != 0xff000000)}
		{
			const float xoffset = data.width / 2.f;
			const float yoffset = data.height / 2.f;
			const float zoffset = data.depth / 2.f;
			debug_assert(xoffset > 0.f);
			debug_assert(yoffset > 0.f);
			debug_assert(zoffset > 0.f);

			std::size_t i = 0;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
		}

		cuboid_c & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};
	const std::array<uint16_t, 3 * 12> cuboid_c::triangles = {{
			0,  1,  3,
			0,  3,  2,
			4,  5,  7,
			4,  7,  6,
			8,  9, 11,
			8, 11, 10,
			12, 13, 15,
			12, 15, 14,
			16, 17, 19,
			16, 19, 18,
			20, 21, 23,
			20, 23, 22
		}};
	const std::array<float, 3 * 24> cuboid_c::normals = {{
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
			0.f, +1.f, 0.f
		}};

	// the vertices of cuboid_cw are numbered as followed:
	//          2----3
	//         /|   /|
	//        6----7 |
	//        | 0--|-1
	//   y    |/   |/
	//   |    4----5
	//   |
	//   +----x
	//  /
	// z
	struct cuboid_cw
	{
		core::maths::Matrix4x4f modelview;
		std::array<float, 3 * 8> vertices;
		engine::graphics::opengl::Color4ub color;

		static const std::array<uint16_t, 2 * 12> lines;

		cuboid_cw(engine::graphics::data::CuboidC && data) :
			modelview(std::move(data.modelview)),
			color((data.color & 0x000000ff) >>  0,
			      (data.color & 0x0000ff00) >>  8,
			      (data.color & 0x00ff0000) >> 16,
			      (data.color & 0xff000000) >> 24)
		{
			const float xoffset = data.width / 2.f;
			const float yoffset = data.height / 2.f;
			const float zoffset = data.depth / 2.f;
			debug_assert(xoffset > 0.f);
			debug_assert(yoffset > 0.f);
			debug_assert(zoffset > 0.f);

			std::size_t i = 0;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
		}

		cuboid_cw & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};
	const std::array<uint16_t, 2 * 12> cuboid_cw::lines = {{
			0, 1,
			0, 2,
			0, 4,
			1, 3,
			1, 5,
			2, 3,
			2, 6,
			3, 7,
			4, 5,
			4, 6,
			5, 7,
			6, 7
		}};

	// the vertices of cuboid_t are numbered as followed:
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
	struct cuboid_t
	{
		core::maths::Matrix4x4f modelview;
		std::array<float, 3 * 24> vertices;
		const texture_t * texture;

		static const std::array<uint16_t, 3 * 12> triangles;
		static const std::array<float, 3 * 24> normals;
		static const std::array<float, 2 * 24> coords;

		cuboid_t(engine::graphics::data::CuboidT && data) :
			modelview(std::move(data.modelview)),
			texture(&materials.get<texture_t>(data.texture))
		{
			const float xoffset = data.width / 2.f;
			const float yoffset = data.height / 2.f;
			const float zoffset = data.depth / 2.f;
			debug_assert(xoffset > 0.f);
			debug_assert(yoffset > 0.f);
			debug_assert(zoffset > 0.f);

			std::size_t i = 0;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = -zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = -yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = +zoffset;
			vertices[i++] = +xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
			vertices[i++] = -xoffset; vertices[i++] = +yoffset; vertices[i++] = -zoffset;
		}

		cuboid_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};
	const std::array<uint16_t, 3 * 12> cuboid_t::triangles = {{
			0,  1,  3,
			0,  3,  2,
			4,  5,  7,
			4,  7,  6,
			8,  9, 11,
			8, 11, 10,
			12, 13, 15,
			12, 15, 14,
			16, 17, 19,
			16, 19, 18,
			20, 21, 23,
			20, 23, 22
		}};
	const std::array<float, 3 * 24> cuboid_t::normals = {{
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
			0.f, +1.f, 0.f
		}};
	const std::array<float, 2 * 24> cuboid_t::coords = {{
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
			1.f, 1.f,
		}};

	struct linec_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer edges;
		engine::graphics::opengl::Color4ub color;

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

	struct mesh_c
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		engine::graphics::opengl::Color4ub color;

		mesh_c(engine::graphics::data::MeshC && data) :
			vertices(std::move(data.vertices)),
			triangles(std::move(data.triangles)),
			normals(std::move(data.normals)),
			color((data.color&0x000000ff)>>0,
			      (data.color&0x0000ff00)>>8,
			      (data.color&0x00ff0000)>>16,
			      (data.color&0xff000000)>>24)
		{}

		mesh_c & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	struct mesh_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		core::container::Buffer coords;
		const texture_t * texture;

		mesh_t(engine::graphics::data::MeshT && data) :
			vertices(std::move(data.vertices)),
			triangles(std::move(data.triangles)),
			normals(std::move(data.normals)),
			coords(std::move(data.coords)),
			texture(&materials.get<texture_t>(data.texture))
		{}

		mesh_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	struct asset_definition_t
	{
		std::vector<mesh_c> meshs;

		asset_definition_t(engine::graphics::renderer::asset_definition_t & d)
		{
			for (auto & mesh : d.meshs)
			{
				this->meshs.emplace_back(std::move(mesh));
			}
		}
	};

	struct asset_instance_t
	{
		asset_definition_t* definition;
		core::maths::Matrix4x4f modelview;

		asset_instance_t(asset_definition_t & definition, core::maths::Matrix4x4f modelview)
			: definition(&definition)
			, modelview(modelview)
		{}

		asset_instance_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	struct Character
	{
		struct Mesh
		{
			core::maths::Matrix4x4f modelview;
			core::container::Buffer vertices;
			core::container::Buffer triangles;
			core::container::Buffer normals;
			core::container::Buffer coords;
			const texture_t * texture;

			Mesh(engine::model::mesh_t && data)
				: modelview(data.matrix)
				, vertices(std::move(data.xyz))
				, triangles(std::move(data.triangles))
				, normals(std::move(data.normals))
				, coords(std::move(data.uv))
				, texture(&materials.get<texture_t>(data.texture))
			{
			}
		};

		struct SetMesh
		{
			Mesh & mesh;

			SetMesh(Mesh & mesh) : mesh(mesh) {}
		};

		Mesh *mesh;

		core::maths::Matrix4x4f modelview;
		std::vector<core::maths::Matrix4x4f> matrix_pallet;
		std::vector<core::maths::Vector4f> vertices;

		Character(SetMesh && data) :
			mesh(&data.mesh),
			modelview(core::maths::Matrix4x4f::identity()),
			vertices(data.mesh.vertices.size())
		{}

		Character & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			this->modelview = std::move(data.matrix);
			return *this;
		}
		Character & operator = (engine::graphics::renderer::CharacterSkinning && data)
		{
			this->matrix_pallet = std::move(data.matrix_pallet);
			return *this;
		}

		void draw()
		{
			const Mesh * const mesh = this->mesh;

			mesh->texture->enable();

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(3, // TODO
				GL_FLOAT, // TODO
				0,
				mesh->vertices.data());
			glNormalPointer(
				GL_FLOAT, // TODO
				0,
				mesh->normals.data());
			glTexCoordPointer(2, // TODO
				GL_FLOAT, // TODO
				0,
				mesh->coords.data());
			glDrawElements(GL_TRIANGLES,
				mesh->triangles.count(),
				GL_UNSIGNED_SHORT, // TODO
				mesh->triangles.data());
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			mesh->texture->disable();
		}
		void update()
		{
		}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		203,
		std::array<Character::Mesh, 50>,
		std::array<asset_definition_t, 50>
	>
	resources;

	core::container::Collection
	<
		engine::Entity,
		1601,
		std::array<Character, 100>,
		std::array<cuboid_c, 100>,
		std::array<cuboid_cw, 100>,
		std::array<cuboid_t, 100>,
		std::array<linec_t, 100>,
		std::array<mesh_c, 100>,
		std::array<mesh_t, 100>,
		std::array<asset_instance_t, 100>
	>
	components;
}

namespace
{
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera2D> queue_notify_camera2d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera3D> queue_notify_camera3d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Viewport> queue_notify_viewport;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Cursor> queue_notify_cursor;

	core::container::CircleQueueSRMW<std::pair<engine::Asset,
	                                           core::graphics::Image>,
	                                 100> queue_register_texture;

	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::CuboidC>,
	                                 100> queue_add_cuboidc;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::CuboidT>,
	                                 100> queue_add_cuboidt;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::LineC>,
	                                 10> queue_add_linec;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::MeshC>,
	                                 100> queue_add_meshc;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::MeshT>,
	                                 100> queue_add_mesht;
	core::container::CircleQueueSRMW<std::pair<engine::Asset,
	                                           engine::model::mesh_t>,
	                                 100> queue_add_character_model;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::renderer::asset_instance_t>,
	                                 100> queue_add_character_instance;

	core::container::CircleQueueSRMW<engine::Entity,
	                                 100> queue_remove;

	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::ModelviewMatrix>,
	                                 100> queue_update_modelviewmatrix;
	core::container::CircleQueueSRSW<std::pair<engine::Entity,
	                                           engine::graphics::renderer::CharacterSkinning>,
	                                 100> queue_update_characterskinning;

	core::container::CircleQueueSRSW<std::tuple<int, int, engine::Entity>, 10> queue_select;

	core::container::CircleQueueSRSW<std::pair<engine::Asset,
	                                           engine::graphics::renderer::asset_definition_t>,
	                                 100> queue_asset_definitions;
	core::container::CircleQueueSRSW<std::pair<engine::Entity,
	                                           engine::graphics::renderer::asset_instance_t>,
	                                 100> queue_asset_instances;

	void poll_add_queue()
	{
		std::pair<engine::Entity,
			engine::graphics::data::CuboidC> message_add_cuboidc;
		while (queue_add_cuboidc.try_pop(message_add_cuboidc))
		{
			if (!message_add_cuboidc.second.wireframe)
			{
				components.emplace<cuboid_c>(message_add_cuboidc.first,
					std::move(message_add_cuboidc.second));
			}
			else
			{
				components.emplace<cuboid_cw>(message_add_cuboidc.first,
					std::move(message_add_cuboidc.second));
			}
		}
		std::pair<engine::Entity,
			engine::graphics::data::CuboidT> message_add_cuboidt;
		while (queue_add_cuboidt.try_pop(message_add_cuboidt))
		{
			components.emplace<cuboid_t>(message_add_cuboidt.first,
				std::move(message_add_cuboidt.second));
		}
		std::pair<engine::Entity,
			engine::graphics::data::LineC> message_add_linec;
		while (queue_add_linec.try_pop(message_add_linec))
		{
			components.add(message_add_linec.first,
				std::move(message_add_linec.second));
		}
		std::pair<engine::Entity,
			engine::graphics::data::MeshC> message_add_meshc;
		while (queue_add_meshc.try_pop(message_add_meshc))
		{
			components.emplace<mesh_c>(message_add_meshc.first,
				std::move(message_add_meshc.second));
		}
		std::pair<engine::Entity,
			engine::graphics::data::MeshT> message_add_mesht;
		while (queue_add_mesht.try_pop(message_add_mesht))
		{
			components.emplace<mesh_t>(message_add_mesht.first,
				std::move(message_add_mesht.second));
		}

		{
			std::pair<engine::Asset, engine::model::mesh_t> data;
			while (queue_add_character_model.try_pop(data))
			{
				debug_assert(!resources.contains(data.first));
				resources.add(data.first, Character::Mesh{ std::move(data.second) });
			}
		}
		{
			std::pair<engine::Entity, engine::graphics::renderer::asset_instance_t> data;
			while (queue_add_character_instance.try_pop(data))
			{
				auto & mesh = resources.get<Character::Mesh>(data.second.asset);
				components.add(data.first, Character::SetMesh{mesh});
				components.update(data.first, engine::graphics::data::ModelviewMatrix{ data.second.modelview } );
			}
		}
		{
			std::pair<engine::Asset, engine::graphics::renderer::asset_definition_t> data;
			while (queue_asset_definitions.try_pop(data))
			{
				resources.emplace<asset_definition_t>(data.first, data.second);
			}
		}
		{
			std::pair<engine::Entity, engine::graphics::renderer::asset_instance_t> data;
			while (queue_asset_instances.try_pop(data))
			{
				auto & definition = resources.get<asset_definition_t>(data.second.asset);
				components.emplace<asset_instance_t>(data.first, definition, data.second.modelview);
			}
		}
	}
	void poll_remove_queue()
	{
		engine::Entity entity;
		while (queue_remove.try_pop(entity))
		{
			// TODO: remove assets that no one uses any more
			components.remove(entity);
		}
	}
	void poll_update_queue()
	{
		std::pair<engine::Entity,
		          engine::graphics::data::ModelviewMatrix> message_update_modelviewmatrix;
		while (queue_update_modelviewmatrix.try_pop(message_update_modelviewmatrix))
		{
			if (components.contains(message_update_modelviewmatrix.first))
			{
				components.update(message_update_modelviewmatrix.first,
				                  std::move(message_update_modelviewmatrix.second));
			}
			else
			{
				// TODO: bring back when all physics object has a rendered shape
			//	debug_printline(0xffffffff, "WARNING no component for entity ", message_update_modelviewmatrix.first);
			}
		}
		std::pair<engine::Entity,
		          engine::graphics::renderer::CharacterSkinning> message_update_characterskinning;
		while (queue_update_characterskinning.try_pop(message_update_characterskinning))
		{
			components.update(message_update_characterskinning.first,
			                  std::move(message_update_characterskinning.second));
		}
	}
}

namespace
{
	core::maths::Matrix4x4f projection2D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f projection3D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f view2D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f view3D = core::maths::Matrix4x4f::identity();

	int viewport_x = 0;
	int viewport_y = 0;
	int viewport_width = 0;
	int viewport_height = 0;
	int cursor_x = -1;
	int cursor_y = -1;

	Stack modelview_matrix;

	engine::graphics::opengl::Font normal_font;

	GLuint framebuffer;
	GLuint entitybuffers[2]; // color, depth
	std::vector<uint32_t> entitypixels;
	std::atomic<int> entitytoggle;
	engine::Entity highlighted_entity = engine::Entity::null();
	engine::graphics::opengl::Color4ub highlighted_color{255, 191, 64, 255};

	engine::Entity get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = viewport_height - 1 - sy;

		if (x >= viewport_x &&
		    y >= viewport_y &&
		    x < viewport_x + viewport_width &&
		    y < viewport_y + viewport_height)
		{
			const unsigned int color = entitypixels[x + y * viewport_width];
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
		GLfloat lightKa[] = { .2f, .2f, .2f, 1.0f };  // ambient light
		GLfloat lightKd[] = { .7f, .7f, .7f, 1.0f };  // diffuse light
		GLfloat lightKs[] = { 1, 1, 1, 1 };           // specular light
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

		// position the light
		float lightPos[4] = { 0, 0, 20, 1 }; // positional light
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glEnable(GL_LIGHT0);
	}

	void render_setup()
	{
		graphics_debug_trace("render_callback starting");
		engine::application::window::make_current();

		graphics_debug_trace("glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		graphics_debug_trace("glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		graphics_debug_trace("glGetString GL_VERSION: ", glGetString(GL_VERSION));

		engine::graphics::opengl::init();

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		initLights();

		// entity buffers
		glGenFramebuffers(1, &framebuffer);
		glGenRenderbuffers(2, entitybuffers);

		// vvvvvvvv tmp vvvvvvvv
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load("consolas", 12))
			{
				debug_fail();
			}
			normal_font.compile(data);

			data.free();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
		core::graphics::Image image{"res/box.png"};
		engine::graphics::renderer::post_register_texture(engine::Asset{"my_png"}, image);

		core::graphics::Image image2{ "res/dude.png" };
		engine::graphics::renderer::post_register_texture(engine::Asset{ "dude" }, image2);

		engine::graphics::renderer::add(engine::Entity::create(), engine::graphics::data::CuboidT{ core::maths::Matrix4x4f::translation(0.f, 5.f, 0.f), 1.f, 1.f, 1.f, engine::Asset{ "my_png" } });
	}

	void render_update()
	{
		//
		std::pair<engine::Asset,
		          core::graphics::Image> message_register_texture;
		while (queue_register_texture.try_pop(message_register_texture))
		{
			materials.emplace<texture_t>(message_register_texture.first,
			                             std::move(message_register_texture.second));
		}
		// poll events
		poll_add_queue();
		poll_update_queue();
		poll_remove_queue();
		//
		// read notifications
		//
		engine::graphics::renderer::Camera2D notification_camera2d;
		if (queue_notify_camera2d.try_pop(notification_camera2d))
		{
			projection2D = notification_camera2d.projection;
			view2D = notification_camera2d.view;
		}
		engine::graphics::renderer::Camera3D notification_camera3d;
		if (queue_notify_camera3d.try_pop(notification_camera3d))
		{
			projection3D = notification_camera3d.projection;
			view3D = notification_camera3d.view;
		}
		engine::graphics::renderer::Viewport notification_viewport;
		if (queue_notify_viewport.try_pop(notification_viewport))
		{
			viewport_x = notification_viewport.x;
			viewport_y = notification_viewport.y;

			glViewport(notification_viewport.x, notification_viewport.y, notification_viewport.width, notification_viewport.height);

			if (viewport_width != notification_viewport.width ||
			    viewport_height != notification_viewport.height)
			{
				viewport_width = notification_viewport.width;
				viewport_height = notification_viewport.height;

				// free old render buffers
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0); // color
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0); // depth
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				glDeleteRenderbuffers(2, entitybuffers);
				// allocate new render buffers
				glGenRenderbuffers(2, entitybuffers);
				glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[0]); // color
				glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, notification_viewport.width, notification_viewport.height);
				glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[1]); // depth
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, notification_viewport.width, notification_viewport.height);

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, entitybuffers[0]); // color
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, entitybuffers[1]); // depth
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				entitypixels.resize(notification_viewport.width * notification_viewport.height);
			}
		}
		engine::graphics::renderer::Cursor notification_cursor;
		if (queue_notify_cursor.try_pop(notification_cursor))
		{
			cursor_x = notification_cursor.x;
			cursor_y = notification_cursor.y;
		}

		glStencilMask(0x000000ff);
		// setup frame
		glClearColor(0.f, 0.f, .1f, 0.f);
		glClearDepth(1.);
		//glClearStencil(0x00000000);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection3D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(view3D);

		////////////////////////////////////////////////////////
		//
		//  entity buffer
		//
		////////////////////////////////////////
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		// glViewport(...);
		glClearColor(0.f, 0.f, 0.f, 0.f); // null entity
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);

		for (const auto & component : components.get<cuboid_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			engine::Entity entity = components.get_key(component);
			engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
			                                            (entity & 0x0000ff00) >> 8,
			                                            (entity & 0x00ff0000) >> 16,
			                                            (entity & 0xff000000) >> 24};
			glColor(color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                GL_FLOAT,
			                0,
			                component.vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.size(),
			               GL_UNSIGNED_SHORT,
			               component.triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<cuboid_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			engine::Entity entity = components.get_key(component);
			engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
			                                            (entity & 0x0000ff00) >> 8,
			                                            (entity & 0x00ff0000) >> 16,
			                                            (entity & 0xff000000) >> 24};
			glColor(color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                GL_FLOAT,
			                0,
			                component.vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.size(),
			               GL_UNSIGNED_SHORT,
			               component.triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<linec_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			engine::Entity entity = components.get_key(component);
			engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
			                                            (entity & 0x0000ff00) >> 8,
			                                            (entity & 0x00ff0000) >> 16,
			                                            (entity & 0xff000000) >> 24};

			glLineWidth(2.f);
			glColor(color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glDrawElements(GL_LINES,
			               component.edges.count(),
			               static_cast<GLenum>(component.edges.format()),
			               component.edges.data());
			glDisableClientState(GL_VERTEX_ARRAY);
			glLineWidth(1.f);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<mesh_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			engine::Entity entity = components.get_key(component);
			engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
			                                            (entity & 0x0000ff00) >> 8,
			                                            (entity & 0x00ff0000) >> 16,
			                                            (entity & 0xff000000) >> 24};

			glColor(color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.count(),
			               static_cast<GLenum>(component.triangles.format()),
			               component.triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<mesh_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			engine::Entity entity = components.get_key(component);
			engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
			                                            (entity & 0x0000ff00) >> 8,
			                                            (entity & 0x00ff0000) >> 16,
			                                            (entity & 0xff000000) >> 24};

			glColor(color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.count(),
			               static_cast<GLenum>(component.triangles.format()),
			               component.triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<asset_instance_t>())
		{
			modelview_matrix.push();
			{
				modelview_matrix.mult(component.modelview);
				glLoadMatrix(modelview_matrix);

				engine::Entity entity = components.get_key(component);
				engine::graphics::opengl::Color4ub color = {(entity & 0x000000ff) >> 0,
				                                            (entity & 0x0000ff00) >> 8,
				                                            (entity & 0x00ff0000) >> 16,
				                                            (entity & 0xff000000) >> 24};

				glColor(color);
				glEnableClientState(GL_VERTEX_ARRAY);
				for (const auto & mesh : component.definition->meshs)
				{
					glVertexPointer(3, // TODO
					                static_cast<GLenum>(mesh.vertices.format()), // TODO
					                0,
					                mesh.vertices.data());
					glDrawElements(GL_TRIANGLES,
					               mesh.triangles.count(),
					               static_cast<GLenum>(mesh.triangles.format()),
					               mesh.triangles.data());
				}
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			modelview_matrix.pop();
		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

		glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, entitypixels.data());

		// hover
		{
			const engine::Entity entity = get_entity_at_screen(cursor_x, cursor_y);
			if (highlighted_entity != entity)
			{
				highlighted_entity = entity;
			}
		}
		// select
		{
			std::tuple<int, int, engine::Entity> select_args;
			while (queue_select.try_pop(select_args))
			{
				gameplay::gamestate::post_command(std::get<2>(select_args), engine::Command::RENDER_SELECT, get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)));
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////
		// wireframes
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glDisable(GL_LIGHTING);

		glStencilMask(0x00000001);
		// glStencilFunc(GL_NEVER, 0x00000001, 0x00000001);
		// glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0x00000001, 0x00000001);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		for (const auto & component : components.get<cuboid_cw>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glLineWidth(1.f);
			glColor(component.color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                GL_FLOAT,
			                0,
			                component.vertices.data());
			glDrawElements(GL_LINES,
			               component.lines.size(),
			               GL_UNSIGNED_SHORT,
			               component.lines.data());
			glDisableClientState(GL_VERTEX_ARRAY);
			glLineWidth(1.f);

			modelview_matrix.pop();
		}

		glEnable(GL_LIGHTING);

		//////////////////////////////////////////////////////////////
		// solids
		glStencilMask(0x00000000);
		glStencilFunc(GL_EQUAL, 0x00000000, 0x00000001);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		for (auto & component : components.get<Character>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			modelview_matrix.mult(component.mesh->modelview);
			glLoadMatrix(modelview_matrix);

			component.update();
			component.draw();

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<cuboid_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			if (component.is_transparent)
			{
				glEnable(GL_BLEND);
				//glDisable(GL_DEPTH_TEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);
			}
			// glEnable(GL_POLYGON_OFFSET_FILL); // necessary for wireframe
			// glPolygonOffset(2.f, 2.f);

			glColor(components.get_key(component) == highlighted_entity ? highlighted_color : component.color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glVertexPointer(3, // TODO
			                GL_FLOAT,
			                0,
			                component.vertices.data());
			glNormalPointer(GL_FLOAT,
			                0,
			                component.normals.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.size(),
			               GL_UNSIGNED_SHORT,
			               component.triangles.data());
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			// glDisable(GL_POLYGON_OFFSET_FILL);
			if (component.is_transparent)
			{
				//glEnable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
			}

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<cuboid_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			if (components.get_key(component) == highlighted_entity)
			{
				glColor(highlighted_color);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(3,
				                GL_FLOAT,
				                0,
				                component.vertices.data());
				glNormalPointer(GL_FLOAT,
				                0,
				                component.normals.data());
				glDrawElements(GL_TRIANGLES,
				               component.triangles.size(),
				               GL_UNSIGNED_SHORT,
				               component.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				component.texture->enable();

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(3,
				                GL_FLOAT,
				                0,
				                component.vertices.data());
				glNormalPointer(GL_FLOAT,
				                0,
				                component.normals.data());
				glTexCoordPointer(2,
				                  GL_FLOAT,
				                  0,
				                  component.coords.data());
				glDrawElements(GL_TRIANGLES,
				               component.triangles.size(),
				               GL_UNSIGNED_SHORT,
				               component.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);

				component.texture->disable();
			}

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<linec_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glLineWidth(2.f);
			glColor(components.get_key(component) == highlighted_entity ? highlighted_color : component.color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glDrawElements(GL_LINES,
			               component.edges.count(),
			               static_cast<GLenum>(component.edges.format()),
			               component.edges.data());
			glDisableClientState(GL_VERTEX_ARRAY);
			glLineWidth(1.f);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<mesh_c>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glColor(components.get_key(component) == highlighted_entity ? highlighted_color : component.color);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glNormalPointer(static_cast<GLenum>(component.normals.format()), // TODO
			                0,
			                component.normals.data());
			glDrawElements(GL_TRIANGLES,
			               component.triangles.count(),
			               static_cast<GLenum>(component.triangles.format()),
			               component.triangles.data());
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<mesh_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			if (components.get_key(component) == highlighted_entity)
			{
				glColor(highlighted_color);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(3, // TODO
				                static_cast<GLenum>(component.vertices.format()), // TODO
				                0,
				                component.vertices.data());
				glNormalPointer(static_cast<GLenum>(component.normals.format()), // TODO
				                0,
				                component.normals.data());
				glDrawElements(GL_TRIANGLES,
				               component.triangles.count(),
				               static_cast<GLenum>(component.triangles.format()),
				               component.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				component.texture->enable();

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(3, // TODO
				                static_cast<GLenum>(component.vertices.format()), // TODO
				                0,
				                component.vertices.data());
				glNormalPointer(static_cast<GLenum>(component.normals.format()), // TODO
				                0,
				                component.normals.data());
				glTexCoordPointer(2, // TODO
				                  static_cast<GLenum>(component.vertices.format()), // TODO
				                  0,
				                  component.coords.data());
				glDrawElements(GL_TRIANGLES,
				               component.triangles.count(),
				               static_cast<GLenum>(component.triangles.format()),
				               component.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);

				component.texture->disable();
			}

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<asset_instance_t>())
		{
			modelview_matrix.push();
			{
				modelview_matrix.mult(component.modelview);
				glLoadMatrix(modelview_matrix);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				for (const auto & mesh : component.definition->meshs)
				{
					glColor(components.get_key(component) == highlighted_entity ? highlighted_color : mesh.color);

					glVertexPointer(3, // TODO
					                static_cast<GLenum>(mesh.vertices.format()), // TODO
					                0,
					                mesh.vertices.data());
					glNormalPointer(static_cast<GLenum>(mesh.normals.format()), // TODO
					                0,
					                mesh.normals.data());
					glDrawElements(GL_TRIANGLES,
					               mesh.triangles.count(),
					               static_cast<GLenum>(mesh.triangles.format()),
					               mesh.triangles.data());
				}
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			modelview_matrix.pop();
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		// entity buffers
		if (entitytoggle.load(std::memory_order_relaxed))
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

			glBlitFramebuffer(viewport_x, viewport_y, viewport_width, viewport_height, viewport_x, viewport_y, viewport_width, viewport_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection2D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(core::maths::Matrix4x4f::identity());

		// draw gui
		// ...
		glLoadMatrix(modelview_matrix);
		glColor3ub(255, 255, 255);
		glRasterPos2i(10, 10 + 12);
		normal_font.draw("herp derp herp derp herp derp herp derp herp derp etc.");
		// 2d
		// ...

		// swap buffers
		engine::application::window::swap_buffers();
	}

	void render_teardown()
	{
		graphics_debug_trace("render_callback stopping");
		// vvvvvvvv tmp vvvvvvvv
		{
			normal_font.decompile();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);
	}

	core::async::Thread renderThread;
	volatile bool active = true;
	core::sync::Event<true> event;

	void run()
	{
		render_setup();

		event.wait();
		event.reset();

		while (active)
		{
			render_update();

			event.wait();
			event.reset();
		}

		render_teardown();
	}
}

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void create()
			{
				renderThread = core::async::Thread{ run };
			}

			void update()
			{
				event.set();
			}

			void destroy()
			{
				active = false;
				event.set();

				renderThread.join();
			}

			void notify(Camera2D && data)
			{
				queue_notify_camera2d.try_push(std::move(data));
			}
			void notify(Camera3D && data)
			{
				queue_notify_camera3d.try_push(std::move(data));
			}
			void notify(Viewport && data)
			{
				queue_notify_viewport.try_push(std::move(data));
			}
			void notify(Cursor && data)
			{
				queue_notify_cursor.try_push(std::move(data));
			}

			void post_register_texture(engine::Asset asset, const core::graphics::Image & image)
			{
				const auto res = queue_register_texture.try_push(std::make_pair(asset, image));
				debug_assert(res);
			}

			void add(engine::Entity entity, data::CuboidC data)
			{
				const auto res = queue_add_cuboidc.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void add(engine::Entity entity, data::CuboidT data)
			{
				const auto res = queue_add_cuboidt.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void add(engine::Entity entity, data::LineC data)
			{
				const auto res = queue_add_linec.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void add(engine::Entity entity, data::MeshC data)
			{
				const auto res = queue_add_meshc.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void add(engine::Entity entity, data::MeshT data)
			{
				const auto res = queue_add_mesht.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void add(engine::Asset asset, engine::model::mesh_t && data)
			{
				const auto res = queue_add_character_model.try_push(std::make_pair(asset, std::move(data)));
				debug_assert(res);
			}
			void add_character_instance(engine::Entity entity, const asset_instance_t & data)
			{
				const auto res = queue_add_character_instance.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}

			void add(engine::Asset asset, const asset_definition_t & data)
			{
				const auto res = queue_asset_definitions.try_push(std::make_pair(asset, data));
				debug_assert(res);
			}
			void add(engine::Entity entity, const asset_instance_t & data)
			{
				const auto res = queue_asset_instances.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}

			void remove(engine::Entity entity)
			{
				const auto res = queue_remove.try_push(entity);
				debug_assert(res);
			}
			void update(engine::Entity entity, data::ModelviewMatrix data)
			{
				const auto res = queue_update_modelviewmatrix.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}
			void update(engine::Entity entity, CharacterSkinning data)
			{
				const auto res = queue_update_characterskinning.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}

			void post_select(int x, int y, engine::Entity entity)
			{
				const auto res = queue_select.try_emplace(x, y, entity);
				debug_assert(res);
			}

			void toggle_down()
			{
				entitytoggle.fetch_add(1, std::memory_order_relaxed);
			}
			void toggle_up()
			{
				entitytoggle.fetch_sub(1, std::memory_order_relaxed);
			}
		}
	}
}
