
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/color.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/Stack.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/debug.hpp>
#include <engine/extras/Asset.hpp>
#include <engine/graphics/Camera.hpp>

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

	namespace physics
	{
		void render();
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
	// the vertices of a cuboid is numbered as followed:
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
	struct cuboidc_t
	{
		core::maths::Matrix4x4f modelview;
		std::array<float, 3 * 24> vertices;
		// engine::graphics::opengl::Color color;

		static const std::array<uint16_t, 3 * 12> triangles;
		static const std::array<float, 3 * 24> normals;

		cuboidc_t(engine::graphics::data::CuboidC && data) :
			modelview(std::move(data.modelview))
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
		cuboidc_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};
	const std::array<uint16_t, 3 * 12> cuboidc_t::triangles = {{
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
	const std::array<float, 3 * 24> cuboidc_t::normals = {{
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

	struct linec_t
	{
		// core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer edges;
		// engine::graphics::opengl::Color color;

		linec_t(engine::graphics::data::LineC && data) :
			vertices(std::move(data.vertices)),
			edges(std::move(data.edges))
		{}
	};

	struct meshc_t
	{
		// core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		// engine::graphics::opengl::Color color;

		meshc_t(engine::graphics::data::MeshC && data) :
			vertices(std::move(data.vertices)),
			triangles(std::move(data.triangles)),
			normals(std::move(data.normals))
		{}
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

		std::vector<core::maths::Matrix4x4f> matrix_pallet;
		std::vector<core::maths::Vector4f> vertices;

		Character(SetMesh && data) :
			mesh(&data.mesh),
			vertices(data.mesh.nvertices)
		{}
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
		std::array<int, 0>
	>
	resources;

	core::container::Collection
	<
		engine::Entity,
		1001,
		std::array<Character, 100>,
		std::array<cuboidc_t, 100>,
		std::array<linec_t, 100>,
		std::array<meshc_t, 100>
	>
	components;
}

namespace
{
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

	void poll_add_queue()
	{
		std::pair<engine::Entity,
		          engine::graphics::data::CuboidC> message_add_cuboidc;
		while (queue_add_cuboidc.try_pop(message_add_cuboidc))
		{
			components.add(message_add_cuboidc.first,
			               std::move(message_add_cuboidc.second));
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
			components.add(message_add_meshc.first,
			               std::move(message_add_meshc.second));
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
			components.update(message_update_modelviewmatrix.first,
			                  std::move(message_update_modelviewmatrix.second));
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
	struct dimension_t
	{
		int32_t width, height;
	};

	dimension_t resizes[3];
	bool masks[3] = {false, false, false};
	int read_resize = 0;
	int write_resize = 1;
	std::atomic_int latest_resize{2};

	/** Current window dimensions */
	dimension_t dimension = {100, 100}; // initialized to something positive

	core::maths::Matrix4x4f projection2D;
	core::maths::Matrix4x4f projection3D;

	core::maths::Matrix4x4f view3D;
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

	void render_update(const engine::graphics::Camera & camera)
	{
		// handle notifications
		read_resize = latest_resize.exchange(read_resize); // std::memory_order_acquire?
		if (masks[read_resize])
		{
			dimension = resizes[read_resize];
			masks[read_resize] = false;

			glViewport(0, 0, dimension.width, dimension.height);

			// these calculations do not need opengl context
			projection2D = core::maths::Matrix4x4f::ortho(0.f, (float)dimension.width, (float)dimension.height, 0.f, -1.f, 1.f);
			projection3D = core::maths::Matrix4x4f::perspective(core::maths::make_degree(80.f), float(dimension.width) / float(dimension.height), .125f, 128.f);
		}
		// poll events
		poll_add_queue();
		poll_update_queue();
		poll_remove_queue();

		// setup frame
		glClearColor(0.f, 0.f, .1f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection3D);
		glMatrixMode(GL_MODELVIEW);
		// vvvvvvvv tmp vvvvvvvv
		view3D = core::maths::Matrix4x4f::translation(-camera.getX(), -camera.getY(), -camera.getZ());
		// ^^^^^^^^ tmp ^^^^^^^^
		modelview_matrix.load(view3D);

		// 3d
		glEnable(GL_DEPTH_TEST);
		//
		for (auto & component : components.get<Character>())
		{
			glLoadMatrix(modelview_matrix);

			component.update();
			component.draw();
		}
		for (const auto & component : components.get<cuboidc_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glColor3ub(0, 255, 0);
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

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<linec_t>())
		{
			glLoadMatrix(modelview_matrix);

			glLineWidth(2.f);
			glColor3ub(255, 0, 0);
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
		}
		for (const auto & component : components.get<meshc_t>())
		{
			glLoadMatrix(modelview_matrix);

			glColor3ub(0, 255, 255);
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

		// TEMP
		glLoadMatrix(modelview_matrix);
		engine::physics::render();
		glLoadMatrix(modelview_matrix);

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

		// something temporary that delays
		core::async::delay(10);

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
}

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void create()
			{
				render_setup();
			}

			void update(const Camera & camera)
			{
				render_update(camera);
			}

			void destroy()
			{
				render_teardown();
			}

			void notify_resize(const int width, const int height)
			{
				resizes[write_resize].width = width;
				resizes[write_resize].height = height;
				masks[write_resize] = true;
				write_resize = latest_resize.exchange(write_resize); // std::memory_order_release?
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
