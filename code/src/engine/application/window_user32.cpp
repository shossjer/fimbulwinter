#include "config.h"

#if WINDOW_USE_USER32

#include "window.hpp"

#include "engine/application/config.hpp"
#include "engine/debug.hpp"

#include "ful/point.hpp"

#if TEXT_USE_USER32
# include "ful/cstr.hpp"
#endif

#include <windowsx.h>
#include <windows.h>
#if HAVE_VERSIONHELPERS_H
# include <versionhelpers.h>
#endif

namespace engine
{
	namespace graphics
	{
		extern void notify_resize(viewer & viewer, int width, int height);
	}

	namespace hid
	{
#if INPUT_HAS_USER32_RAWINPUT
		extern void add_device(devices & devices, HANDLE device);
		extern void remove_device(devices & devices, HANDLE device);
		extern void process_input(devices & devices, HRAWINPUT input);
#endif

		extern void key_character(devices & devices, int scancode, ful::point_utf character);

		extern void key_down(devices & devices, WPARAM wParam, LPARAM lParam, LONG time);
		extern void key_up(devices & devices, WPARAM wParam, LPARAM lParam, LONG time);
		extern void syskey_down(devices & devices, WPARAM wParam, LPARAM lParam, LONG time);
		extern void syskey_up(devices & devices, WPARAM wParam, LPARAM lParam, LONG time);

		extern void lbutton_down(devices & devices, LONG time);
		extern void lbutton_up(devices & devices, LONG time);
		extern void mbutton_down(devices & devices, LONG time);
		extern void mbutton_up(devices & devices, LONG time);
		extern void rbutton_down(devices & devices, LONG time);
		extern void rbutton_up(devices & devices, LONG time);
		extern void mouse_move(devices & devices, int_fast16_t x, int_fast16_t y, LONG time);
		extern void mouse_wheel(devices & devices, int_fast16_t delta, LONG time);

		extern void notify_resize(ui & ui, const int width, const int height);
	}
}

namespace
{
	engine::graphics::viewer * viewer = nullptr;
	engine::hid::devices * devices = nullptr;
	engine::hid::ui * ui = nullptr;

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
	LRESULT CALLBACK WinProc(HWND hWnd_, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
#if INPUT_HAS_USER32_RAWINPUT
		case WM_INPUT:
			if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
			{
				process_input(*::devices, reinterpret_cast<HRAWINPUT>(lParam));
			}
			return DefWindowProcW(hWnd_, msg, wParam, lParam);
		case WM_INPUT_DEVICE_CHANGE:
			switch (wParam)
			{
			case GIDC_ARRIVAL: add_device(*::devices, reinterpret_cast<HANDLE>(lParam)); break;
			case GIDC_REMOVAL: remove_device(*::devices, reinterpret_cast<HANDLE>(lParam)); break;
			default: debug_unreachable();
			}
			break;
#endif

		case WM_UNICHAR:
			if (wParam == UNICODE_NOCHAR)
				return TRUE;
			else
				key_character(*::devices, uint32_t(lParam & 0xff0000) >> 16, ful::point_utf(static_cast<ful::uint32>(wParam)));
			break;

		case WM_KEYDOWN:
			key_down(*::devices, wParam, lParam, GetMessageTime());
			break;
		case WM_KEYUP:
			key_up(*::devices, wParam, lParam, GetMessageTime());
			break;
		case WM_SYSKEYDOWN:
			syskey_down(*::devices, wParam, lParam, GetMessageTime());
			break;
		case WM_SYSKEYUP:
			syskey_up(*::devices, wParam, lParam, GetMessageTime());
			break;

		case WM_MOUSEMOVE:
			mouse_move(*::devices, (int_fast16_t)GET_X_LPARAM(lParam), (int_fast16_t)GET_Y_LPARAM(lParam), GetMessageTime());
			break;
		case WM_LBUTTONDOWN:
			lbutton_down(*::devices, GetMessageTime());
			break;
		case WM_LBUTTONUP:
			lbutton_up(*::devices, GetMessageTime());
			break;
		case WM_RBUTTONDOWN:
			rbutton_down(*::devices, GetMessageTime());
			break;
		case WM_RBUTTONUP:
			rbutton_up(*::devices, GetMessageTime());
			break;
		case WM_MBUTTONDOWN:
			mbutton_down(*::devices, GetMessageTime());
			break;
		case WM_MBUTTONUP:
			mbutton_up(*::devices, GetMessageTime());
			break;
		case WM_MOUSEWHEEL:
			mouse_wheel(*::devices, (int_fast16_t)HIWORD(wParam), GetMessageTime());
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			notify_resize(*::viewer, (int_fast16_t)LOWORD(lParam), (int_fast16_t)HIWORD(lParam));
			notify_resize(*::ui, (int_fast16_t)LOWORD(lParam), (int_fast16_t)HIWORD(lParam));
			break;

		default:
			return DefWindowProcW(hWnd_, msg, wParam, lParam);
		}
		return 0;
	}
	/**
	 */
	inline int messageLoop()
	{
		MSG msg;

		while (GetMessageW(&msg, nullptr, 0, 0)) // while the message isn't 'WM_QUIT'
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		return (int)msg.wParam;
	}

	void RegisterRawInputDevicesWithFlags(const uint32_t * collections, int count, DWORD dwFlags, HWND hWnd_)
	{
		RAWINPUTDEVICE rids[10]; // arbitrary
		debug_assert(std::size_t(count) < sizeof rids / sizeof rids[0]);

		for (int i = 0; i < count; i++)
		{
			rids[i].usUsagePage = collections[i] >> 16;
			rids[i].usUsage = collections[i] & 0x0000ffff;
			rids[i].dwFlags = dwFlags;
			rids[i].hwndTarget = hWnd_;
		}

		if (RegisterRawInputDevices(rids, count, sizeof rids[0]) == FALSE)
		{
			const auto err = GetLastError();
			debug_fail("RegisterRawInputDevices failed: ", err);
		}
	}

	HINSTANCE cast(void * hInstance)
	{
		return static_cast<HINSTANCE>(hInstance);
	}
}

namespace engine
{
	namespace application
	{
		window::~window()
		{
			wglDeleteContext(hGLRC);

			ReleaseDC(hWnd, hDC);
			DestroyWindow(hWnd);
			UnregisterClassW(L"Tribunal Window Class Name", cast(hInstance_));

			::ui = nullptr;
			::devices = nullptr;
			::viewer = nullptr;
		}

		// TODO: proper error handling
		window::window(HINSTANCE hInstance, int nCmdShow, const config_t & config)
			: hInstance_(hInstance)
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
			                             cast(hInstance),
			                             LoadIconW(0, IDI_APPLICATION),
			                             LoadCursorW(nullptr, IDC_ARROW),
			                             (HBRUSH)COLOR_WINDOW,
			                             0,
			                             L"Tribunal Window Class Name",
			                             LoadIconW(cast(hInstance), IDI_APPLICATION)};
			RegisterClassExW(&WndClass);
			// create window
			::hWnd = CreateWindowExW(WS_EX_CLIENTEDGE,
			                         L"Tribunal Window Class Name",
			                         L"Tribunal \U00010348 \u2603",
			                         WS_OVERLAPPEDWINDOW,
			                         CW_USEDEFAULT,
			                         CW_USEDEFAULT,
			                         config.window_width,
			                         config.window_height,
			                         0,
			                         0,
			                         cast(hInstance),
			                         0);
			// create window graphics
			::hDC = GetDC(::hWnd);

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
			const int pf = ChoosePixelFormat(::hDC, &pfd);

			SetPixelFormat(::hDC, pf, &pfd);

			::hGLRC = wglCreateContext(::hDC);

			ShowWindow(::hWnd, nCmdShow);
		}

		void window::set_dependencies(engine::graphics::viewer & viewer_, engine::hid::devices & devices_, engine::hid::ui & ui_)
		{
			::viewer = &viewer_;
			::devices = &devices_;
			::ui = &ui_;
		}

		void make_current(window & /*window*/)
		{
			wglMakeCurrent(hDC, hGLRC);
		}

		void swap_buffers(window & /*window*/)
		{
			SwapBuffers(hDC);
		}

#if INPUT_HAS_USER32_RAWINPUT
		void RegisterRawInputDevices(window & /*window*/, const uint32_t * collections, int count)
		{
			RegisterRawInputDevicesWithFlags(collections, count, RIDEV_DEVNOTIFY, hWnd);
		}

		void UnregisterRawInputDevices(window & /*window*/, const uint32_t * collections, int count)
		{
			RegisterRawInputDevicesWithFlags(collections, count, RIDEV_REMOVE, nullptr);
		}
#endif

#if TEXT_USE_USER32
		void buildFont(window & /*window*/, HFONT hFont, DWORD count, DWORD listBase)
		{
			HGDIOBJ hPrevious = SelectObject(hDC, hFont);

			wglUseFontBitmaps(hDC, 0, count, listBase);

			SelectObject(hDC, hPrevious);
		}

		void freeFont(window & /*window*/, HFONT hFont)
		{
			DeleteObject(hFont);
		}

		HFONT loadFont(window & /*window*/, utility::string_units_utf8 name, int height)
		{
			return CreateFontW(height,
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
			                   utility::heap_widen(name).data());
		}
#endif

		int execute(window & /*window*/)
		{
			return messageLoop();
		}

		void close()
		{
			PostMessageW(hWnd, WM_CLOSE, 0, 0);
		}
	}
}

#endif /* WINDOW_USE_USER32 */
