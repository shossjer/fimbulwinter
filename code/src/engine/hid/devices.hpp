#pragma once

namespace engine
{
	namespace application
	{
		class window;
	}

	namespace hid
	{
		class ui;
	}
}

namespace engine
{
	namespace hid
	{
		class devices
		{
		public:
			~devices();
			devices(engine::application::window & window, engine::hid::ui & ui, bool hardware_input);
		};
	}
}
