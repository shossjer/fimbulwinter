
#include "renderer.hpp"

#include <config.h>

#include <core/color.hpp>
#include <core/maths/Matrix.hpp>
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>
#include <engine/debug.hpp>

#include <atomic>
#include <stdexcept>
#include <utility>

#if WINDOW_USE_USER32
# include <windows.h>
# include <GL/gl.h>
# include <GL/glext.h>
#elif WINDOW_USE_X11
# include <GL/gl.h>
#endif

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

	dimension_t resizes[3] = {{100, 100}, {100, 100}, {100, 100}}; // random data
	int read_resize = 0;
	int write_resize = 1;
	std::atomic_int latest_resize{2};

	dimension_t dimension = {100, 100}; // current dimension used by the engine

	core::maths::Matrixf projection2D;
	core::maths::Matrixf projection3D;

	void render_callback()
	{
		graphics_debug_trace("render_callback starting");
		engine::application::window::make_current();

		graphics_debug_trace("glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		graphics_debug_trace("glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		graphics_debug_trace("glGetString GL_VERSION: ", glGetString(GL_VERSION));
		graphics_debug_trace("glGetString GL_SHADING_LANGUAGE_VERSION: ", glGetString(GL_SHADING_LANGUAGE_VERSION));

		// pre-loop stuff
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);

		while (active)
		{
			// handle notifications
			read_resize = latest_resize.exchange(read_resize); // std::memory_order_acquire?
			if (dimension.width != resizes[read_resize].width ||
			    dimension.height != resizes[read_resize].height)
			{
				dimension = resizes[read_resize];

				glViewport(0, 0, dimension.width, dimension.height);

				// these calculations do not need opengl context
				projection2D = core::maths::Matrixf::ortho(0., dimension.width, dimension.height, 0., -1., 1.);
				projection3D = core::maths::Matrixf::perspective(100., float(dimension.width) / float(dimension.height), 1., 100.);
			}
			// setup frame
			static core::color::hsv_t<float> tmp1{0, 1, 1};
			tmp1 += core::color::hue_t<float>{1};
			const auto tmp2 = make_rgb(tmp1);
			glClearColor(tmp2.red(), tmp2.green(), tmp2.blue(), 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// setup 3D
			glMatrixMode(GL_PROJECTION);
			// glLoadMatrixf(projection3D);
			glMatrixMode(GL_MODELVIEW);

			// glLoadMatrixd(_view_matrix_3d.get());

			glEnable(GL_DEPTH_TEST);

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
				write_resize = latest_resize.exchange(write_resize); // std::memory_order_release?
			}
		}
	}
}
