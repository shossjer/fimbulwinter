
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>
#include <core/color.hpp>
#include <core/container/Stack.hpp>
#include <core/maths/Matrix.hpp>
#include <engine/debug.hpp>

#include <atomic>
#include <stdexcept>
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
namespace engine
{
	namespace physics
	{
		extern void init();
		extern void draw();
		extern void simulate(const double dt);
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
			debug_assert(this->stack.size() > 1);
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
	struct dimension_t
	{
		int32_t width, height;
	};

	std::atomic_bool active;
	core::async::Thread render_thread;

	dimension_t resizes[3];
	bool masks[3] = {false, false, false};
	int read_resize = 0;
	int write_resize = 1;
	std::atomic_int latest_resize{2};

	/* Current dimension used by the engine */
	dimension_t dimension = {100, 100}; // initialized to something positive

	core::maths::Matrix4x4f projection2D;
	core::maths::Matrix4x4f projection3D;

	core::maths::Matrix4x4f view3D;
	// core::container::Stack<core::maths::Matrix4x4f, 10> modelview_stack;
	Stack modelview_matrix;

	engine::graphics::opengl::Font normal_font;

	void render_callback()
	{
		graphics_debug_trace("render_callback starting");
		engine::application::window::make_current();

		graphics_debug_trace("glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		graphics_debug_trace("glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		graphics_debug_trace("glGetString GL_VERSION: ", glGetString(GL_VERSION));
#if WINDOW_USE_X11
		graphics_debug_trace("glGetString GL_SHADING_LANGUAGE_VERSION: ", glGetString(GL_SHADING_LANGUAGE_VERSION));
#endif

		// pre-loop stuff
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);
		// vvvvvvvv tmp vvvvvvvv
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load("consolas", 12))
			{
				debug_assert(("COULD NOT LOAD THE FONT: maybe it is missing?", false));
			}
			normal_font.compile(data);

			data.free();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
		engine::physics::init(); // WAT

		while (active)
		{
			// handle notifications
			read_resize = latest_resize.exchange(read_resize); // std::memory_order_acquire?
			if (masks[read_resize])
			{
				dimension = resizes[read_resize];
				masks[read_resize] = false;

				glViewport(0, 0, dimension.width, dimension.height);

				// these calculations do not need opengl context
				projection2D = core::maths::Matrix4x4f::ortho(0., dimension.width, dimension.height, 0., -1., 1.);
				projection3D = core::maths::Matrix4x4f::perspective(core::maths::make_degree(80.f), float(dimension.width) / float(dimension.height), .125f, 128.f);
			}
			// setup frame
			glClearColor(0.f, 0.f, .1f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// setup 3D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection3D);
			glMatrixMode(GL_MODELVIEW);
			// vvvvvvvv tmp vvvvvvvv
			view3D = core::maths::Matrix4x4f::translation(0.f, -50.f, -64.f);
			// ^^^^^^^^ tmp ^^^^^^^^
			modelview_matrix.load(view3D);

			glEnable(GL_DEPTH_TEST);

			glLoadMatrix(modelview_matrix);
			engine::physics::simulate(1. / 100.);
			engine::physics::draw(); // WAT

			// setup 2D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection2D);
			glMatrixMode(GL_MODELVIEW);
			modelview_matrix.load(core::maths::Matrix4x4f::identity());

			glDisable(GL_DEPTH_TEST);

			// draw gui
			// ...
			glLoadMatrix(modelview_matrix);
			glColor3ub(255, 255, 255);
			glRasterPos2i(10, 10 + 12);
			normal_font.draw("herp derp herp derp herp derp herp derp herp derp etc.");

			core::async::delay(10);

			// swap buffers
			engine::application::window::swap_buffers();
		}
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
				active = true;

				core::async::Thread render_thread{render_callback};

				::render_thread = std::move(render_thread);
			}
			void destroy()
			{
				active = false;

				render_thread.join();
			}

			void notify_resize(const int width, const int height)
			{
				resizes[write_resize].width = width;
				resizes[write_resize].height = height;
				masks[write_resize] = true;
				write_resize = latest_resize.exchange(write_resize); // std::memory_order_release?
			}
		}
	}
}
