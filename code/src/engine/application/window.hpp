#pragma once

#include "config.h"

namespace engine
{
	namespace application
	{
		struct config_t;
	}

	namespace graphics
	{
		class viewer;
	}

	namespace hid
	{
		class devices;
		class ui;
	}
}

namespace engine
{
	namespace application
	{
		class window
		{
#if WINDOW_USE_USER32
			// hack around the problem of having to include WinDef.h
			using HINSTANCE = void *;

		private:
			HINSTANCE hInstance_;
#endif

		public:
			~window();
#if WINDOW_USE_USER32
			window(HINSTANCE hInstance, int nCmdShow, const config_t & config);
#else
			window(const config_t & config);
#endif

		public:
			void set_dependencies(engine::graphics::viewer & viewer, engine::hid::devices & devices, engine::hid::ui & ui);
		};

		void close();
	}
}
