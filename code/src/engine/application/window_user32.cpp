
#include <config.h>

#if WINDOW_USE_USER32

#include "window.hpp"

#include "engine/debug.hpp"

#include <windowsx.h>
#include <windows.h>
#if HAVE_VERSIONHELPERS_H
# include <versionhelpers.h>
#endif

namespace engine
{
	namespace graphics
	{
		namespace viewer
		{
			extern void notify_resize(const int width, const int height);
		}
	}
	namespace hid
	{
#if INPUT_USE_RAWINPUT
		extern void add_device(HANDLE device);
		extern void remove_device(HANDLE device);
		extern void process_input(HRAWINPUT input);
#endif

		extern void key_character(int scancode, const char16_t * character);

#if !INPUT_USE_RAWINPUT
		extern void key_down(WPARAM wParam, LPARAM lParam, LONG time);
		extern void key_up(WPARAM wParam, LPARAM lParam, LONG time);
		extern void syskey_down(WPARAM wParam, LPARAM lParam, LONG time);
		extern void syskey_up(WPARAM wParam, LPARAM lParam, LONG time);

		extern void lbutton_down(LONG time);
		extern void lbutton_up(LONG time);
		extern void mbutton_down(LONG time);
		extern void mbutton_up(LONG time);
		extern void rbutton_down(LONG time);
		extern void rbutton_up(LONG time);
		extern void mouse_move(const int_fast16_t x,
		                       const int_fast16_t y,
		                       LONG time);
		extern void mouse_wheel(const int_fast16_t delta,
		                        LONG time);
#endif

		namespace ui
		{
			extern void notify_resize(const int width, const int height);
		}
	}
}

namespace
{
	/**
	 */
	HWND hWnd;
	/**
	 */
	HDC hDC;
	/**
	 */
	HGLRC hGLRC;

	/**
	 */
	LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
#if INPUT_USE_RAWINPUT
		case WM_INPUT:
			if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
			{
				engine::hid::process_input(reinterpret_cast<HRAWINPUT>(lParam));
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		case WM_INPUT_DEVICE_CHANGE:
			switch (wParam)
			{
			case GIDC_ARRIVAL: engine::hid::add_device(reinterpret_cast<HANDLE>(lParam)); break;
			case GIDC_REMOVAL: engine::hid::remove_device(reinterpret_cast<HANDLE>(lParam)); break;
			default: debug_unreachable();
			}
			break;
#endif

		 case WM_CHAR:
			engine::hid::key_character(uint32_t(lParam & 0xff0000) >> 16, reinterpret_cast<const char16_t *>(&wParam));
			break;

#if !INPUT_USE_RAWINPUT
		case WM_KEYDOWN:
			engine::hid::key_down(wParam, lParam, GetMessageTime());
			break;
		case WM_KEYUP:
			engine::hid::key_up(wParam, lParam, GetMessageTime());
			break;
		case WM_SYSKEYDOWN:
			engine::hid::syskey_down(wParam, lParam, GetMessageTime());
			break;
		case WM_SYSKEYUP:
			engine::hid::syskey_up(wParam, lParam, GetMessageTime());
			break;

		case WM_MOUSEMOVE:
			engine::hid::mouse_move((int_fast16_t)GET_X_LPARAM(lParam), (int_fast16_t)GET_Y_LPARAM(lParam), GetMessageTime());
			break;
		case WM_LBUTTONDOWN:
			engine::hid::lbutton_down(GetMessageTime());
			break;
		case WM_LBUTTONUP:
			engine::hid::lbutton_up(GetMessageTime());
			break;
		case WM_RBUTTONDOWN:
			engine::hid::rbutton_down(GetMessageTime());
			break;
		case WM_RBUTTONUP:
			engine::hid::rbutton_up(GetMessageTime());
			break;
		case WM_MBUTTONDOWN:
			engine::hid::mbutton_down(GetMessageTime());
			break;
		case WM_MBUTTONUP:
			engine::hid::mbutton_up(GetMessageTime());
			break;
		case WM_MOUSEWHEEL:
			engine::hid::mouse_wheel((int_fast16_t)HIWORD(wParam), GetMessageTime());
			break;
#endif

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			engine::graphics::viewer::notify_resize((int_fast16_t)LOWORD(lParam),
			                                        (int_fast16_t)HIWORD(lParam));
			engine::hid::ui::notify_resize((int_fast16_t)LOWORD(lParam),
			                               (int_fast16_t)HIWORD(lParam));
			break;

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}
	/**
	 */
	inline int messageLoop()
	{
		MSG msg;

		while (GetMessage(&msg, nullptr, 0, 0)) // while the message isn't 'WM_QUIT'
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (int)msg.wParam;
	}
}

namespace engine
{
	namespace application
	{
		namespace window
		{
			// TODO: proper error handling
			void create(HINSTANCE hInstance, int nCmdShow, const config_t & config)
			{
				OSVERSIONINFO osvi;
				{
					ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
					osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				}

				//GetVersionEx(&osvi);
				//debug_printline(engine::application_channel, "GetVersionEx: ", osvi.dwMajorVersion, " ", osvi.dwMinorVersion);

				// register window class
				const WNDCLASSEX WndClass = {sizeof(WNDCLASSEX),
				                             0,
				                             WinProc,
				                             0,
				                             0,
				                             hInstance,
				                             LoadIcon(0, IDI_APPLICATION),
				                             LoadCursor(nullptr, IDC_ARROW),
				                             (HBRUSH)COLOR_WINDOW,
				                             0,
				                             "Tribunal Window Class Name",
				                             LoadIcon(hInstance, IDI_APPLICATION)};
				RegisterClassEx(&WndClass);
				// create window
				HWND hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
				                           "Tribunal Window Class Name",
				                           "Tribunal",
				                           WS_OVERLAPPEDWINDOW,
				                           CW_USEDEFAULT,
				                           CW_USEDEFAULT,
				                           config.window_width,
				                           config.window_height,
				                           0,
				                           0,
				                           hInstance,
				                           0);
				// create window graphics
				HDC hDC = GetDC(hWnd);

				const PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
				                                   0,
				                                   PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
				                                   PFD_TYPE_RGBA,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   32,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0,
				                                   0};
				const int pf = ChoosePixelFormat(hDC, &pfd);

				SetPixelFormat(hDC, pf, &pfd);

				HGLRC hGLRC = wglCreateContext(hDC);

				ShowWindow(hWnd, nCmdShow);

				::hWnd = hWnd;
				::hDC = hDC;
				::hGLRC = hGLRC;
			}
			void destroy(HINSTANCE hInstance)
			{
				wglDeleteContext(hGLRC);

				ReleaseDC(hWnd, hDC);
				DestroyWindow(hWnd);
				UnregisterClass("Tribunal Window Class Name", hInstance);
			}

			void make_current()
			{
				wglMakeCurrent(hDC, hGLRC);
			}
			void swap_buffers()
			{
				SwapBuffers(hDC);
			}

#if INPUT_USE_RAWINPUT
			void RegisterRawInputDevices(const uint32_t * collections, int count)
			{
				RAWINPUTDEVICE rids[10]; // arbitrary
				debug_assert(count < sizeof rids / sizeof rids[0]);

				for (int i = 0; i < count; i++)
				{
					rids[i].usUsagePage = collections[i] >> 16;
					rids[i].usUsage = collections[i] & 0x0000ffff;
					rids[i].dwFlags = RIDEV_DEVNOTIFY;
					rids[i].hwndTarget = hWnd;
				}

				if (RegisterRawInputDevices(rids, count, sizeof rids[0]) == FALSE)
				{
					const auto err = GetLastError();
					debug_fail("RegisterRawInputDevices failed: ", err);
				}
			}
#endif

#if TEXT_USE_USER32
			void buildFont(HFONT hFont, const DWORD count, const DWORD listBase)
			{
				HGDIOBJ hPrevious = SelectObject(hDC, hFont);

				wglUseFontBitmaps(hDC, 0, count, listBase);

				SelectObject(hDC, hPrevious);
			}
			void freeFont(HFONT hFont)
			{
				DeleteObject(hFont);
			}
			HFONT loadFont(const char *const name, const int height)
			{
				return CreateFont(height,
				                  0,
				                  0,
				                  0,
				                  FW_DONTCARE,
				                  FALSE,
				                  FALSE,
				                  FALSE,
				                  DEFAULT_CHARSET,
				                  OUT_OUTLINE_PRECIS,
				                  CLIP_DEFAULT_PRECIS,
				                  5, // CLEARTYPE_QUALITY,
				                  VARIABLE_PITCH,
				                  TEXT(name));
			}
#endif

			int execute()
			{
				return messageLoop();
			}

			void close()
			{
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			}
		}
	}
}

#endif /* WINDOW_USE_USER32 */
