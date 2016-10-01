
#include <engine/debug.hpp>

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
		extern void key_down(BYTE virtual_key,
		                     BYTE scan_code,
		                     LONG time);
		extern void key_up(BYTE virtual_key,
		                   BYTE scan_code,
		                   LONG time);
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

		extern void syskey_down(BYTE virtual_key,
		                        BYTE scan_code,
		                        LONG time);
		extern void syskey_up(BYTE virtual_key,
		                      BYTE scan_code,
		                      LONG time);
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
			// case WM_CHAR:
			// 	break;
			// case WM_DEADCHAR:
			// 	break;
		case WM_KEYDOWN:
			engine::hid::key_down((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
		case WM_KEYUP:
			engine::hid::key_up((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
			// case WM_SYSDEADCHAR:
			// 	break;
		case WM_SYSKEYDOWN:
			engine::hid::syskey_down((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
		case WM_SYSKEYUP:
			engine::hid::syskey_up((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
			// case WM_UNICHAR:
			// 	break;

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
			// cronus::core::dispatch_wheel((int)((int16_t)HIWORD(wParam) / WHEEL_DELTA), GetMessageTime());
			engine::hid::mouse_wheel((int_fast16_t)HIWORD(wParam), GetMessageTime());
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			engine::graphics::viewer::notify_resize((int_fast16_t)LOWORD(lParam),
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
			void create(HINSTANCE hInstance, const int nCmdShow)
			{
				OSVERSIONINFO osvi;
				{
					ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
					osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				}

				//GetVersionEx(&osvi);
				//application_debug_trace("GetVersionEx: ", osvi.dwMajorVersion, " ", osvi.dwMinorVersion);

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
				                           800,
				                           400,
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
