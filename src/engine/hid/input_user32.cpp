
#include <config.h>

#if WINDOW_USE_USER32

#include "input.hpp"

namespace engine
{
	namespace hid
	{
		extern void dispatch(const Input & input);
	}
}

namespace
{
	engine::hid::Input input;
}

namespace engine
{
	namespace hid
	{
		void key_down(BYTE virtual_key,
		              BYTE scan_code,
		              LONG time)
		{
			input.setDown(virtual_key, scan_code);

			dispatch(input);
		}
		void key_up(BYTE virtual_key,
		            BYTE scan_code,
		            LONG time)
		{
			input.setUp(virtual_key, scan_code);

			dispatch(input);
		}
		// void lbutton_down(LONG time)
		// {
		// 	input.setDown(0x1, 0);

		// 	dispatch(input);
		// }
		// void lbutton_up(LONG time)
		// {
		// 	input.setUp(0x1, 0);

		// 	dispatch(input);
		// }
		// void mbutton_down(LONG time)
		// {
		// 	input.setDown(0x4, 0);

		// 	dispatch(input);
		// }
		// void mbutton_up(LONG time)
		// {
		// 	input.setUp(0x4, 0);

		// 	dispatch(input);
		// }
		void mouse_move(const int_fast16_t x,
		                const int_fast16_t y,
		                LONG time)
		{
			input.setMove(x - input.getCursor().x,
			              y - input.getCursor().y);
			input.setCursor(x, y);

			dispatch(input);
		}
		// void mouse_wheel(const int_fast16_t delta,
		//                  LONG time)
		// {
		// 	// TODO:
		// }
		// void rbutton_down(LONG time)
		// {
		// 	input.setDown(0x2, 0);

		// 	dispatch(input);
		// }
		// void rbutton_up(LONG time)
		// {
		// 	input.setUp(0x2, 0);

		// 	dispatch(input);
		// }
		void syskey_down(BYTE virtual_key,
		                 BYTE scan_code,
		                 LONG time)
		{
			input.setDown(virtual_key, scan_code);

			dispatch(input);
		}
		void syskey_up(BYTE virtual_key,
		               BYTE scan_code,
		               LONG time)
		{
			input.setUp(virtual_key, scan_code);

			dispatch(input);
		}
	}
}

#endif /* WINDOW_USE_USER32 */
