
#ifndef ENGINE_APPLICATION_WINDOW_HPP
#define ENGINE_APPLICATION_WINDOW_HPP

#include "config.hpp"

#include "config.h"

#if WINDOW_USE_USER32
# include <windows.h>
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
			void create(HINSTANCE hInstance, int nCmdShow, const config_t & config);
			/**
			 */
			void destroy(HINSTANCE hInstance);
#elif WINDOW_USE_X11
			/**
			 */
			void create(const config_t & config);
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
