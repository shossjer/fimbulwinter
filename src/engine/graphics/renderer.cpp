
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/color.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/container/Stack.hpp>

#include <engine/Collection.hpp>
#include <engine/debug.hpp>
#include <engine/graphics/Camera.hpp>

#include <atomic>
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

	namespace model
	{
		extern void init();
		extern void draw();
		extern void update();
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

		cuboidc_t & operator = (engine::graphics::data::CuboidC && data)
		{
			modelview = std::move(data.modelview);

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
			return *this;
		}
		cuboidc_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
		cuboidc_t & operator = (engine::graphics::data::Data && data)
		{
			debug_unreachable();
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

		linec_t & operator = (engine::graphics::data::LineC && data)
		{
			vertices = std::move(data.vertices);
			edges = std::move(data.edges);
			return *this;
		}
		linec_t & operator = (engine::graphics::data::Data && data)
		{
			debug_unreachable();
		}
	};

	struct meshc_t
	{
		// core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		// engine::graphics::opengl::Color color;

		meshc_t & operator = (engine::graphics::data::MeshC && data)
		{
			vertices = std::move(data.vertices);
			triangles = std::move(data.triangles);
			normals = std::move(data.normals);
			return *this;
		}
		meshc_t & operator = (engine::graphics::data::Data && data)
		{
			debug_unreachable();
		}
	};

	engine::Collection
	<
		1001,
		std::array<cuboidc_t, 100>,
		std::array<linec_t, 100>,
		std::array<meshc_t, 100>
	>
	collection;
}

namespace
{
	template <typename T>
	using message_t = std::pair<engine::Entity, T>;

	core::container::CircleQueue<message_t<engine::graphics::data::CuboidC>,
	                             100> add_queue_cuboidc;
	core::container::CircleQueue<message_t<engine::graphics::data::LineC>,
	                             10> add_queue_linec;
	core::container::CircleQueue<message_t<engine::graphics::data::MeshC>,
	                             100> add_queue_meshc;

	core::container::CircleQueue<engine::Entity,
	                             100> remove_queue;

	core::container::CircleQueue<message_t<engine::graphics::data::ModelviewMatrix>,
	                             100> update_queue_modelviewmatrix;


	void poll_add_queue()
	{
		message_t<engine::graphics::data::CuboidC> add_msg_cuboidc;
		while (add_queue_cuboidc.try_pop(add_msg_cuboidc))
		{
			collection.add(add_msg_cuboidc.first,
			               std::move(add_msg_cuboidc.second));
		}
		message_t<engine::graphics::data::LineC> add_msg_linec;
		while (add_queue_linec.try_pop(add_msg_linec))
		{
			collection.add(add_msg_linec.first,
			               std::move(add_msg_linec.second));
		}
		message_t<engine::graphics::data::MeshC> add_msg_meshc;
		while (add_queue_meshc.try_pop(add_msg_meshc))
		{
			collection.add(add_msg_meshc.first,
			               std::move(add_msg_meshc.second));
		}
	}
	void poll_remove_queue()
	{
		engine::Entity entity;
		while (remove_queue.try_pop(entity))
		{
			collection.remove(entity);
		}
	}
	void poll_update_queue()
	{
		message_t<engine::graphics::data::ModelviewMatrix> update_msg_modelviewmatrix;
		while (update_queue_modelviewmatrix.try_pop(update_msg_modelviewmatrix))
		{
			collection.update(update_msg_modelviewmatrix.first,
			                  std::move(update_msg_modelviewmatrix.second));
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

	// this is totally unsafe!
	const engine::graphics::Camera * activeCamera;

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
		engine::model::init(); // WAT
	}

	void render_update()
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
		view3D = core::maths::Matrix4x4f::translation(-activeCamera->getX(), -activeCamera->getY(), -activeCamera->getZ());
		// ^^^^^^^^ tmp ^^^^^^^^
		modelview_matrix.load(view3D);

		// 3d
		glEnable(GL_DEPTH_TEST);
		//
		for (const auto & component : collection.get<cuboidc_t>())
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
		for (const auto & component : collection.get<linec_t>())
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
		for (const auto & component : collection.get<meshc_t>())
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
		engine::model::update();
		engine::model::draw();
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
			void create(const Camera & camera)
			{
				activeCamera = &camera;

				render_setup();
			}

			void update()
			{
				render_update();
			}

			void destroy2()
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
				add_queue_cuboidc.try_push(message_t<data::CuboidC>{entity, data});
			}
			void add(engine::Entity entity, data::LineC data)
			{
				add_queue_linec.try_push(message_t<data::LineC>{entity, data});
			}
			void add(engine::Entity entity, data::MeshC data)
			{
				add_queue_meshc.try_push(message_t<data::MeshC>{entity, data});
			}
			void remove(engine::Entity entity)
			{
				remove_queue.try_push(entity);
			}
			void update(engine::Entity entity, data::ModelviewMatrix data)
			{
				update_queue_modelviewmatrix.try_push(message_t<data::ModelviewMatrix>{entity, data});
			}
		}
	}
}
