
#include <engine/debug.hpp>

#include <windows.h>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void notify_resize(const int width, const int height);
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
			// case WM_CHAR:
			// 	break;
			// case WM_DEADCHAR:
			// 	break;
		case WM_KEYDOWN:
			// cronus::core::dispatch<cronus::user::State::DOWN>((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
		case WM_KEYUP:
			// cronus::core::dispatch<cronus::user::State::UP>((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
			// case WM_SYSDEADCHAR:
			// 	break;
		case WM_SYSKEYDOWN:
			// cronus::core::dispatch<cronus::user::State::DOWN>((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
		case WM_SYSKEYUP:
			// cronus::core::dispatch<cronus::user::State::UP>((BYTE)wParam, (BYTE)(lParam >> 16), GetMessageTime());
			break;
			// case WM_UNICHAR:
			// 	break;

		case WM_MOUSEMOVE:
			// cronus::core::dispatch<cronus::user::State::MOVE>(cronus::user::Cursor((int_fast16_t)GET_X_LPARAM(lParam), (int_fast16_t)GET_Y_LPARAM(lParam)), GetMessageTime());
			break;
		case WM_LBUTTONDOWN:
			// cronus::core::dispatch<cronus::user::State::DOWN>(0x01, GetMessageTime());
			break;
		case WM_LBUTTONUP:
			// cronus::core::dispatch<cronus::user::State::UP>(0x01, GetMessageTime());
			break;
		case WM_RBUTTONDOWN:
			// cronus::core::dispatch<cronus::user::State::DOWN>(0x02, GetMessageTime());
			break;
		case WM_RBUTTONUP:
			// cronus::core::dispatch<cronus::user::State::UP>(0x02, GetMessageTime());
			break;
		case WM_MBUTTONDOWN:
			// cronus::core::dispatch<cronus::user::State::DOWN>(0x04, GetMessageTime());
			break;
		case WM_MBUTTONUP:
			// cronus::core::dispatch<cronus::user::State::UP>(0x04, GetMessageTime());
			break;
		case WM_MOUSEWHEEL:
			// cronus::core::dispatch_wheel((int)((int16_t)HIWORD(wParam) / WHEEL_DELTA), GetMessageTime());
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			engine::graphics::renderer::notify_resize((int_fast16_t)LOWORD(lParam),
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

				ShowWindow(hWnd);

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
