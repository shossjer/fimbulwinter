
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
#include <core/maths/Matrix.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>
#include <core/sync/Event.hpp>

#include <engine/debug.hpp>
#include <engine/extras/Asset.hpp>

#include <atomic>
#include <fstream>
#include <utility>

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

	struct meshc_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		engine::graphics::opengl::Color4ub color;

		meshc_t(engine::graphics::data::MeshC && data) :
			vertices(std::move(data.vertices)),
			triangles(std::move(data.triangles)),
			normals(std::move(data.normals)),
			color((data.color&0x000000ff)>>0,
			      (data.color&0x0000ff00)>>8,
			      (data.color&0x00ff0000)>>16,
			      (data.color&0xff000000)>>24)
		{}

		meshc_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	struct asset_instance_t
	{
		engine::Entity defId;
		core::maths::Matrix4x4f modelview;

		asset_instance_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	struct asset_definition_t
	{
		std::vector<meshc_t> meshs;

		asset_definition_t(engine::graphics::renderer::asset_definition_t & d)
		{
			for (auto & mesh : d.meshs)
			{
				;
				this->meshs.emplace_back(meshc_t { std::move(mesh) });
			}
		}
	};

	struct Character
	{
		struct Mesh
		{
			struct Weight
			{
				uint16_t index;
				float value;
			};

			char name[64]; // arbitrary

			uint16_t nvertices;
			uint16_t nedges;

			std::vector<core::maths::Vector4f> vertices;
			std::vector<std::pair<uint16_t, uint16_t>> edges;
			std::vector<Weight> weights;
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
			vertices(data.mesh.nvertices)
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
			glColor3ub(255, 0, 255);
			glLineWidth(2.f);
			glBegin(GL_LINES);
			for (auto && edge : mesh->edges)
			{
				core::maths::Vector4f::array_type buffer1;
				vertices[edge.first].get_aligned(buffer1);
				glVertex4fv(buffer1);
				core::maths::Vector4f::array_type buffer2;
				vertices[edge.second].get_aligned(buffer2);
				glVertex4fv(buffer2);
			}
			glEnd();
			glLineWidth(1.f);
		}
		void update()
		{
			debug_assert(!matrix_pallet.empty());
			for (int i = 0; i < static_cast<int>(mesh->nvertices); i++)
				vertices[i] = matrix_pallet[mesh->weights[i].index] * mesh->vertices[i];
		}
	};

	void read_count(std::ifstream & stream, uint16_t & count)
	{
		stream.read(reinterpret_cast<char *>(& count), sizeof(uint16_t));
	}
	void read_vector(std::ifstream & stream, core::maths::Vector4f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set(buffer[0], buffer[1], buffer[2], 1.f);
	}
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (&buffer)[N])
	{
		uint16_t len; // including null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
		debug_assert(len <= N);

		stream.read(buffer, len);
	}

	void read_weight(std::ifstream & stream, Character::Mesh::Weight & weight)
	{
		read_count(stream, weight.index);

		stream.read(reinterpret_cast<char *>(& weight.value), sizeof(float));
	}
	void read_mesh(std::ifstream & stream, Character::Mesh & mesh)
	{
		read_string(stream, mesh.name);
		debug_printline(0xffffffff, "mesh name: ", mesh.name);

		read_count(stream, mesh.nvertices);
		debug_printline(0xffffffff, "mesh nvertices: ", mesh.nvertices);

		mesh.vertices.resize(mesh.nvertices);
		for (auto && vertex : mesh.vertices)
			read_vector(stream, vertex);

		read_count(stream, mesh.nedges);
		debug_printline(0xffffffff, "mesh nedges: ", mesh.nedges);

		mesh.edges.resize(mesh.nedges);
		for (auto && edge : mesh.edges)
		{
			read_count(stream, edge.first);
			read_count(stream, edge.second);
		}

		mesh.weights.resize(mesh.nvertices);
		for (int i = 0; i < static_cast<int>(mesh.nvertices); i++)
		{
			uint16_t ngroups;
			read_count(stream, ngroups);
			debug_assert(ngroups == 1);

			read_weight(stream, mesh.weights[i]);
		}
	}

	core::container::UnorderedCollection
	<
		engine::extras::Asset,
		101,
		std::array<Character::Mesh, 50>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	resources;

	core::container::Collection
	<
		engine::Entity,
		1201,
		std::array<Character, 100>,
		std::array<cuboid_c, 100>,
		std::array<cuboid_cw, 100>,
		std::array<linec_t, 100>,
		std::array<meshc_t, 100>,
		std::array<asset_instance_t, 100>
	>
	components;

	core::container::Collection
	<
		engine::Entity,
		200,
		std::array<asset_definition_t, 100>,
		std::array<int, 1>
	>
	definitions;

}

namespace
{
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera2D> queue_notify_camera2d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera3D> queue_notify_camera3d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Viewport> queue_notify_viewport;

	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::CuboidC>,
	                                 100> queue_add_cuboidc;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::LineC>,
	                                 10> queue_add_linec;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::MeshC>,
	                                 100> queue_add_meshc;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::renderer::asset::CharacterMesh>,
	                                 100> queue_add_charactermesh;

	core::container::CircleQueueSRMW<engine::Entity,
	                                 100> queue_remove;

	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::data::ModelviewMatrix>,
	                                 100> queue_update_modelviewmatrix;
	core::container::CircleQueueSRSW<std::pair<engine::Entity,
	                                           engine::graphics::renderer::CharacterSkinning>,
	                                 100> queue_update_characterskinning;

	core::container::CircleQueueSRSW<std::pair<engine::Entity,
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
		          engine::graphics::data::LineC> message_add_linec;
		while (queue_add_linec.try_pop(message_add_linec))
		{
			components.add(message_add_linec.first,
			               std::move(message_add_linec.second));
		}

		std::pair<engine::Entity,
		          engine::graphics::renderer::asset::CharacterMesh> message_add_charactermesh;
		while (queue_add_charactermesh.try_pop(message_add_charactermesh))
		{
			// TODO: this should be done in a loader thread somehow
			const engine::extras::Asset mshasset{message_add_charactermesh.second.mshfile};
			if (!resources.contains(mshasset))
			{
				std::ifstream file(message_add_charactermesh.second.mshfile, std::ifstream::binary | std::ifstream::in);
				Character::Mesh msh;
				read_mesh(file, msh);
				resources.add(mshasset, std::move(msh));
			}
			components.add(message_add_charactermesh.first, Character::SetMesh{resources.get<Character::Mesh>(mshasset)});
		}
		{
			std::pair<engine::Entity, engine::graphics::renderer::asset_definition_t> data;
			while (queue_asset_definitions.try_pop(data))
			{
				definitions.add(data.first, asset_definition_t{data.second});
			}
		}
		{
			std::pair<engine::Entity, engine::graphics::renderer::asset_instance_t> data;
			while (queue_asset_instances.try_pop(data))
			{
				components.add(
						data.first,
						asset_instance_t{ data.second.defId, data.second.modelview });
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

	Stack modelview_matrix;

	engine::graphics::opengl::Font normal_font;

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
#if WINDOW_USE_X11
		graphics_debug_trace("glGetString GL_SHADING_LANGUAGE_VERSION: ", glGetString(GL_SHADING_LANGUAGE_VERSION));
#endif

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		initLights();

		// vvvvvvvv tmp vvvvvvvv
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load("consolas", 12))
			{
				debug_assert(false);
			}
			normal_font.compile(data);

			data.free();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
	}

	struct render_definition_t
	{
		const asset_instance_t & inst;

		render_definition_t(const asset_instance_t & inst)
			:
			inst(inst)
		{}

		void operator () (asset_definition_t & x)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);

			for (const auto & mesh : x.meshs)
			{
				glColor(mesh.color);

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

		template <typename X>
		void operator () (X & x)
		{
		}
	};

	void render_update()
	{
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
			glViewport(notification_viewport.x, notification_viewport.y, notification_viewport.width, notification_viewport.height);
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

		// 3d
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		// int i;
		// glGetIntegerv(GL_STENCIL_BITS, &i);
		// debug_printline(0xffffffff, i);
		
		//////////////////////////////////////////////////////////////
		// wireframes
		glStencilMask(0x00000001);
		// glStencilFunc(GL_NEVER, 0x00000001, 0x00000001);
		// glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0x00000001, 0x00000001);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		glDisable(GL_LIGHTING);
		
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

			glColor(component.color);
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
		for (const auto & component : components.get<linec_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glLineWidth(2.f);
			glColor(component.color);
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
		for (const auto & component : components.get<meshc_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glColor(component.color);
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
		for (const auto & component : components.get<asset_instance_t>())
		{
			modelview_matrix.push();
			{
				modelview_matrix.mult(component.modelview);
				glLoadMatrix(modelview_matrix);

				definitions.call(component.defId, render_definition_t{ component });
			}
			modelview_matrix.pop();
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);

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

			void add(engine::Entity entity, data::CuboidC data)
			{
				const auto res = queue_add_cuboidc.try_push(std::make_pair(entity, data));
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
			void add(engine::Entity entity, asset::CharacterMesh data)
			{
				const auto res = queue_add_charactermesh.try_push(std::make_pair(entity, data));
				debug_assert(res);
			}

			void add(engine::Entity entity, const asset_definition_t & data)
			{
				const auto res = queue_asset_definitions.try_push(std::make_pair(entity, data));
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
		}
	}
}
