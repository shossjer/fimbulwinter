
#include <engine/debug.hpp>
#include <utility/string.hpp>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <unistd.h>

#include <stdexcept>

// if set to 1 and glx version 1.3 is available, glXUseXFont will produce:
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// X Error of failed request:  BadDrawable (invalid Pixmap or Window parameter)
//   Major opcode of failed request:  53 (X_CreatePixmap)
//   Resource id in failed request:  0x3200004
//   Serial number of failed request:  51
//   Current serial number in output stream:  52
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// possible solution: http://mesa-dev.freedesktop.narkive.com/H6labNrr/patch-fix-for-throwing-baddrawable-invalid-pixmap-or-window-parameter-by-xserver
#define TRY_GLX_VERSION_1_3 0

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
	struct Display_guard
	{
		using resource_t = Display;

		resource_t *resource;

		template <typename ...Ps>
		Display_guard(Ps &&...ps) :
			resource(XOpenDisplay(std::forward<Ps>(ps)...))
			{
				if (this->resource == nullptr)
					throw std::runtime_error("XOpenDisplay failed");
			}
		~Display_guard()
			{
				if (this->resource != nullptr)
					XCloseDisplay(this->resource);
			}

		operator       resource_t *()       { return this->resource; }
		operator const resource_t *() const { return this->resource; }
		      resource_t *operator -> ()       { return this->resource; }
		const resource_t *operator -> () const { return this->resource; }

		resource_t *detach()
			{
				debug_assert(this->resource != nullptr);

				resource_t *const resource = this->resource;
				this->resource = nullptr;
				return resource;
			}
	};
	struct GLXFBConfig_guard
	{
		using resource_t = GLXFBConfig;

		resource_t *resource;

		template <typename ...Ps>
		GLXFBConfig_guard(Ps &&...ps) :
			resource(glXChooseFBConfig(std::forward<Ps>(ps)...))
			{
				if (this->resource == nullptr)
					throw std::runtime_error("glXChooseFBConfig failed");
			}
		~GLXFBConfig_guard()
			{
				if (this->resource != nullptr)
					XFree(this->resource);
			}

		operator       resource_t *()       { return this->resource; }
		operator const resource_t *() const { return this->resource; }
		      resource_t *operator -> ()       { return this->resource; }
		const resource_t *operator -> () const { return this->resource; }

		resource_t *detach()
			{
				debug_assert(this->resource != nullptr);

				resource_t *const tmp = this->resource;
				this->resource = nullptr;
				return tmp;
			}
	};
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
	struct XVisualInfo_guard
	{
		using resource_t = XVisualInfo;

		resource_t *resource;

		template <typename ...Ps>
		XVisualInfo_guard(Ps &&...ps) :
			resource(glXGetVisualFromFBConfig(std::forward<Ps>(ps)...))
			{
				if (this->resource == nullptr)
					throw std::runtime_error("glXGetVisualFromFBConfig failed");
			}
		~XVisualInfo_guard()
			{
				if (this->resource != nullptr)
					XFree(this->resource);
			}

		operator       resource_t *()       { return this->resource; }
		operator const resource_t *() const { return this->resource; }
		      resource_t *operator -> ()       { return this->resource; }
		const resource_t *operator -> () const { return this->resource; }

		resource_t *detach()
			{
				debug_assert(this->resource != nullptr);

				resource_t *const tmp = this->resource;
				this->resource = nullptr;
				return tmp;
			}
	};
#else
	struct XVisualInfo_guard
	{
		using resource_t = XVisualInfo;

		resource_t *resource;

		template <typename ...Ps>
		XVisualInfo_guard(Ps &&...ps) :
			resource(glXChooseVisual(std::forward<Ps>(ps)...))
			{
				if (this->resource == nullptr)
					throw std::runtime_error("glXGetVisualFromFBConfig failed");
			}
		~XVisualInfo_guard()
			{
				if (this->resource != nullptr)
					XFree(this->resource);
			}

		operator       resource_t *()       { return this->resource; }
		operator const resource_t *() const { return this->resource; }
		      resource_t *operator -> ()       { return this->resource; }
		const resource_t *operator -> () const { return this->resource; }

		resource_t *detach()
			{
				debug_assert(this->resource != nullptr);

				resource_t *const tmp = this->resource;
				this->resource = nullptr;
				return tmp;
			}
	};
#endif

	/**
	 */
	Display *event_display;
	/**
	 */
	Display *render_display;
	/**
	 */
	Window render_window;
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
	/**
	 */
	GLXWindow glx_window;
	/**
	 */
	GLXContext glx_context;
#else
	/**
	 */
	GLXContext render_context;
#endif
	/**
	 */
	Atom wm_delete_window;

	/**
	 */
	inline int messageLoop()
	{
		XEvent event;

		while (!XNextEvent(event_display, &event))
		{
			switch (event.type)
			{
			// case ButtonPress:
			// 	// cronus::core::dispatch_button_press(event.xbutton.button,
			// 	//                                     event.xbutton.state,
			// 	//                                     event.xbutton.time);
			// 	break;
			// case ButtonRelease:
			// 	// cronus::core::dispatch_button_release(event.xbutton.button,
			// 	//                                       event.xbutton.state,
			// 	//                                       event.xbutton.time);
			// 	break;
			case ConfigureNotify:
				break;
			// case DestroyNotify:
			// 	break;
			case Expose:
				engine::graphics::renderer::notify_resize(event.xexpose.width,
				                                          event.xexpose.height);
				break;
			// case KeyPress:
			// 	// cronus::core::dispatch_key_press(event.xkey.keycode,
			// 	//                                  event.xkey.state,
			// 	//                                  event.xkey.time);
			// 	break;
			// case KeyRelease:
			// 	// cronus::core::dispatch_key_release(event.xkey.keycode,
			// 	//                                    event.xkey.state,
			// 	//                                    event.xkey.time);
			// 	break;
			case MapNotify:
				break;
			case MotionNotify:
				// cronus::core::dispatch_motion_notify(event.xmotion.x,
				//                                      event.xmotion.y,
				//                                      event.xmotion.time);
				break;
			case ReparentNotify:
				break;
			case UnmapNotify:
				return 0;
			default:
				application_debug_printline("Event type(event_display): ", event.type);
			}
		}
		return -1;
	}
}


namespace engine
{
	namespace application
	{
		namespace window
		{
			void create()
			{
				// XOpenDisplay
				Display_guard event_display(nullptr);
				Display_guard render_display(nullptr);

				// glXQueryExtension
				{
					int errorBase;
					int eventBase;

					if (!glXQueryExtension(render_display, &errorBase, &eventBase))
					{
						throw std::runtime_error("glXQueryExtension: failed");
					}
					application_debug_trace("glXQueryExtension: ", errorBase, " ", eventBase);
				}
				// glXQueryVersion
				{
					int major;
					int minor;

					if (!glXQueryVersion(render_display, &major, &minor))
					{
						throw std::runtime_error("glXQueryVersion: failed");
					}
					application_debug_trace("glXQueryVersion: ", major, " ", minor);
				}
				// XDefaultScreen
				const int screen = XDefaultScreen(render_display);
#if defined(GLX_VERSION_1_1)
				// glXGetClientString
				{
					application_debug_trace("glXGetClientString GLX_VENDOR: ", glXGetClientString(render_display, GLX_VENDOR));
					application_debug_trace("glXGetClientString GLX_VERSION: ", glXGetClientString(render_display, GLX_VERSION));
					// application_debug_trace("glXGetClientString GLX_EXTENSIONS: ", glXGetClientString(render_display, GLX_EXTENSIONS));
				}
				// glXQueryServerString
				{
					application_debug_trace("glXQueryServerString GLX_VENDOR: ", glXQueryServerString(render_display, screen, GLX_VENDOR));
					application_debug_trace("glXQueryServerString GLX_VERSION: ", glXQueryServerString(render_display, screen, GLX_VERSION));
					// application_debug_trace("glXQueryServerString GLX_EXTENSIONS: ", glXQueryServerString(render_display, screen, GLX_EXTENSIONS));
				}
				// glXQueryExtensionsString
				{
					// application_debug_trace("glXQueryExtensionsString: ", glXQueryExtensionsString(render_display, screen));
				}
#endif

#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
				// visual
				int fb_attributes[] = {
					GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
					GLX_RENDER_TYPE,   GLX_RGBA_BIT,
					GLX_DOUBLEBUFFER,  True,
					GLX_RED_SIZE,      1,
					GLX_GREEN_SIZE,    1,
					GLX_BLUE_SIZE,     1,
					None
				};
				int n_buffers;

				GLXFBConfig_guard fb_configs(render_display, screen, fb_attributes, &n_buffers);
				XVisualInfo_guard visual_info(render_display, fb_configs[0]);
				// root
				Window root = XRootWindow(render_display, visual_info->screen);

				XSetWindowAttributes window_attributes;
				{
					window_attributes.colormap = XCreateColormap(render_display,
					                                             root,
					                                             visual_info->visual,
					                                             AllocNone);
					window_attributes.event_mask = NoEventMask;
				}
				Window render_window = XCreateWindow(render_display,
				                                     root,
				                                     0,
				                                     0,
				                                     800,
				                                     400,
				                                     0,
				                                     visual_info->depth,
				                                     InputOutput,
				                                     visual_info->visual,
				                                     CWColormap | CWEventMask,
				                                     &window_attributes);

				GLXContext glx_context = glXCreateNewContext(render_display,
				                                             fb_configs[0],
				                                             GLX_RGBA_TYPE,
				                                             nullptr,
				                                             True);

				GLXWindow glx_window = glXCreateWindow(render_display,
				                                       fb_configs[0],
				                                       render_window,
				                                       nullptr);
#else
				// visual
				GLint visual_attributes[] = {
					GLX_RGBA,
					GLX_DEPTH_SIZE, 24,
					GLX_DOUBLEBUFFER,
					None
				};

				XVisualInfo_guard visual_info(render_display, 0, visual_attributes);
				// root
				Window root = XRootWindow(render_display, screen);

				XSetWindowAttributes window_attributes;
				{
					window_attributes.colormap = XCreateColormap(render_display,
					                                             root,
					                                             visual_info->visual,
					                                             AllocNone);
					window_attributes.event_mask = NoEventMask;
				}
				Window render_window = XCreateWindow(render_display,
				                                     root,
				                                     0,
				                                     0,
				                                     800,
				                                     400,
				                                     0,
				                                     visual_info->depth,
				                                     InputOutput,
				                                     visual_info->visual,
				                                     CWColormap | CWEventMask,
				                                     &window_attributes);

				GLXContext render_context = glXCreateContext(render_display,
				                                             visual_info,
				                                             nullptr,
				                                             True);
#endif
				wm_delete_window = XInternAtom(render_display, "WM_DELETE_WINDOW", False);
				{
					XSetWMProtocols(render_display, render_window, &wm_delete_window, 1);
				}

				XSelectInput(event_display, render_window, ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | StructureNotifyMask);

				XMapWindow(render_display, render_window);
				//
				::event_display = event_display.detach();
				::render_display = render_display.detach();
				::render_window = render_window;
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
				::glx_window = glx_window;
				::glx_context = glx_context;
#else
				::render_context = render_context;
#endif
			}
			void destroy()
			{
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
				glXDestroyWindow(render_display, glx_window);
				glXDestroyContext(render_display, glx_context);
				XDestroyWindow(render_display, render_window);
				XCloseDisplay(render_display);
				XCloseDisplay(event_display);
#else
				glXDestroyContext(render_display, render_context);
				XDestroyWindow(render_display, render_window);
				XCloseDisplay(render_display);
				XCloseDisplay(event_display);
#endif
			}

			void make_current()
			{
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
				glXMakeContextCurrent(render_display, glx_window, glx_window, glx_context);
#else
				glXMakeCurrent(render_display, render_window, render_context);
#endif
			}
			void swap_buffers()
			{
				// check if we are supposed to close down the application
				{
					XEvent event;

					while (XEventsQueued(render_display, 0))
					{
						XNextEvent(render_display, &event);

						switch (event.type)
						{
						case ClientMessage:
							if ((Atom)event.xclient.data.l[0] == wm_delete_window)
							{
								application_debug_printline("wm_delete_window");

								XUnmapWindow(render_display, event.xclient.window);
							}
							break;
						default:
							application_debug_printline("Event type(render_display): ", event.type);
						}
					}
				}
				// swap buffers
#if defined(GLX_VERSION_1_3) && TRY_GLX_VERSION_1_3
				glXSwapBuffers(render_display, glx_window);
#else
				glXSwapBuffers(render_display, render_window);
#endif
			}

			void buildFont(XFontStruct *const font_struct, const unsigned int first, const unsigned int last ,const int base)
			{
				glXUseXFont(font_struct->fid, first, last - first + 1, base + first);
			}
			void freeFont(XFontStruct *const font_struct)
			{
				XFreeFont(render_display, font_struct);
			}
			XFontStruct *loadFont(const char *const name, const int height)
			{
				int n_old_paths;
				char **const old_paths = XGetFontPath(render_display, &n_old_paths);

				char *const pwd = get_current_dir_name();
				{
					const auto directory = utility::concat(pwd, "/res/font");

					const char *const new_paths[] = {directory.c_str()};
					int n_new_paths = 1;
					XSetFontPath(render_display, const_cast<char **>(new_paths), n_new_paths);
				}
				free(pwd);

				const auto format = utility::concat("-*-", name, "-*-*-*-*-*-*-*-", height * 6, "-*-*-iso8859-1");

				XFontStruct *const font_struct = XLoadQueryFont(render_display, format.c_str());

				XSetFontPath(render_display, old_paths, n_old_paths);

				XFreeFontPath(old_paths);

				return font_struct;
			}

			int execute()
			{
				return messageLoop();
			}

			void close()
			{
				application_debug_printline("WARNING: 'cronus::application::close' is not implemented");
			}
		}
	}
}
