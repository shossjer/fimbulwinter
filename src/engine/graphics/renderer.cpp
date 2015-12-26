
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>
#include <core/color.hpp>
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

	core::maths::Matrixf projection2D;
	core::maths::Matrixf projection3D;

	core::maths::Matrixf view3D;

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
				projection2D = core::maths::Matrixf::ortho(0., dimension.width, dimension.height, 0., -1., 1.);
				projection3D = core::maths::Matrixf::perspective(core::maths::make_degree(80.), float(dimension.width) / float(dimension.height), .125, 128.);
			}
			// setup frame
			static core::color::hsv_t<float> tmp1{0, 1, 1};
			tmp1 += core::color::hue_t<float>{1};
			const auto tmp2 = make_rgb(tmp1);
			glClearColor(tmp2.red(), tmp2.green(), tmp2.blue(), 0.f);
			// glClearColor(0.f, 0.f, 0.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// setup 3D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection3D);
			glMatrixMode(GL_MODELVIEW);

			// glLoadMatrixd(_view_matrix_3d.get());
			view3D = core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
			                              0.f, 1.f, 0.f, -1.f,
			                              0.f, 0.f, 1.f, -10.f,
			                              0.f, 0.f, 0.f, 1.f);
			glLoadMatrix(view3D);

			glEnable(GL_DEPTH_TEST);

			static double deg = 0.;
			if ((deg += 1.) >= 360.) deg -= 360.;
			glRotated(deg, 0.f, 1.f, 0.f);
			glBegin(GL_LINES);
			glColor3ub(255, 0, 0);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(100.f, 0.f, 0.f);
			glColor3ub(0, 255, 0);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 100.f, 0.f);
			glColor3ub(0, 0, 255);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 0.f, 100.f);
			glEnd();

			// draw meshes
			// for (int i = 0; i < n_meshes; i++)
			// {
			// 	meshes[i].draw();
			// }

			// setup 2D
			glMatrixMode(GL_PROJECTION);
			// glLoadMatrixf(projection2D);
			glMatrixMode(GL_MODELVIEW);

			glLoadIdentity();

			glDisable(GL_DEPTH_TEST);

			// draw gui
			// ...

			core::async::delay(10);

			// swap buffers
			engine::application::window::swap_buffers();
		}
		graphics_debug_trace("render_callback stopping");
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
