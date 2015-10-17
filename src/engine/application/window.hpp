
#ifndef ENGINE_APPLICATION_WINDOW_HPP
#define ENGINE_APPLICATION_WINDOW_HPP

#include <config.h>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

namespace engine
{
	namespace application
	{
		namespace window
		{
#if WINDOW_USE_USER32
			/**
			 */
			void create(HINSTANCE hInstance, int nCmdShow);
			/**
			 */
			void destroy(HINSTANCE hInstance);
#elif WINDOW_USE_X11
			/**
			 */
			void create();
			/**
			 */
			void destroy();
#endif
			/**
			 */
			int execute();

			/**
			 * Closes the application.
			 *
			 * This shuts down everything by posting a quit message onto the window's event queue.
			 */
			void close();
		}
	}
}

#endif /* ENGINE_APPLICATION_WINDOW_HPP */
