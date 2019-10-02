
#include <config.h>

#if WINDOW_USE_X11

#include "window.hpp"

#include "engine/debug.hpp"
#include "utility/string.hpp"
#include "utility/unicode.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <unistd.h>

#include <atomic>
#include <stdexcept>

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
		extern void key_character(XKeyEvent & event, utility::code_point cp);

		extern void button_press(XButtonEvent & event);
		extern void button_release(XButtonEvent & event);
		extern void key_press(XKeyEvent & event);
		extern void key_release(XKeyEvent & event);
		extern void motion_notify(const int x,
		                          const int y,
		                          const ::Time time);

		namespace ui
		{
			extern void notify_resize(const int width, const int height);
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
#ifdef GLX_VERSION_1_3
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
					throw std::runtime_error("glXChooseVisual failed");
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
#ifdef GLX_VERSION_1_3
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
	XIM input_method = nullptr;
	/**
	 */
	XIC input_context = nullptr;
	/**
	 */
	Atom wm_delete_window;
	/**
	 */
	std::atomic_int should_close_window(0);

	utility::code_point get_unicode(XKeyEvent & event)
	{
		utility::code_point cp(nullptr);
#ifdef X_HAVE_UTF8_STRING
		if (input_context)
		{
			char buffer[4];
			KeySym keysym;
			Status status;
			const int n = Xutf8LookupString(input_context, &event, buffer, sizeof buffer, &keysym, &status);
			debug_assert(status != XBufferOverflow, "4 chars should be enough for any unicode code point");
			if (n > 0)
			{
				cp = utility::code_point(buffer);
			}
			// debug_printline(cp);
		}
#else
# warning "Xutf8LookupString not supported, all unicode key events will be null"
		// try use XmbLookupString?
#endif
		return cp;
	}

	/**
	 */
	inline int messageLoop()
	{
		XEvent event;

		while (!XNextEvent(event_display, &event))
		{
			if (XFilterEvent(&event, None) == True)
			{
				debug_printline("filtererd event type: ", event.type);
				continue;
			}

			switch (event.type)
			{
			case ButtonPress:
				engine::hid::button_press(event.xbutton);
				break;
			case ButtonRelease:
				engine::hid::button_release(event.xbutton);
				break;
			case ConfigureNotify:
				break;
			// case DestroyNotify:
			// 	break;
			case Expose:
				engine::graphics::viewer::notify_resize(event.xexpose.width,
				                                        event.xexpose.height);
				engine::hid::ui::notify_resize(event.xexpose.width,
				                               event.xexpose.height);
				break;
			case KeyPress:
				engine::hid::key_press(event.xkey);
				engine::hid::key_character(event.xkey, get_unicode(event.xkey));
				break;
			case KeyRelease:
				engine::hid::key_release(event.xkey);
				break;
			case MapNotify:
				break;
			case MotionNotify:
				engine::hid::motion_notify(event.xmotion.x,
				                           event.xmotion.y,
				                           event.xmotion.time);
				break;
			case ReparentNotify:
				break;
			case UnmapNotify:
				return 0;
			default:
				debug_printline(engine::application_channel, "Event type(event_display): ", event.type);
			}
		}
		return -1;
	}

	void create_input_method(Display & display)
	{
		debug_assert(!input_method);

		input_method = XOpenIM(&display, nullptr, nullptr, nullptr);

		XIMStyles * styles;
		char * const first_failed = XGetIMValues(input_method, XNQueryInputStyle, &styles, NULL);
		if (first_failed)
		{
			debug_fail("Failed to get XIM styles");
			XCloseIM(input_method);
			input_method = nullptr;
			return;
		}
		debug_assert(styles);

		bool need_for_geometry_management = false;
		for (int i = 0; i < styles->count_styles; i++)
		{
			if ((styles->supported_styles[i] & XIMPreeditArea) ||
			    (styles->supported_styles[i] & XIMStatusArea))
			{
				need_for_geometry_management = true;
			}
			debug_printline("XIM style ", i, ": ", styles->supported_styles[i]);
		}
		if (need_for_geometry_management)
		{
			debug_printline("THERE IS A NEED FOR GEOMETRY MANAGEMENT");
			// The input method indicates the need for geometry management by
			// setting XIMPreeditArea or XIMStatusArea in its XIMStyles value
			// returned by XGetIMValues. When a client has decided that it will
			// provide geometry management for an input method, it indicates that
			// decision by setting the XNInputStyle value in the XIC.
			// :point_up:
			// https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Input_Method_Overview
		}

		XFree(styles);
	}
	void destroy_input_method()
	{
		if (input_method)
		{
			XCloseIM(input_method);
		}
	}

	void create_input_context(Window & window)
	{
		if (!debug_verify(input_method, "input method is needed"))
			return;

		debug_assert(!input_context);

		input_context = XCreateIC(input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window, NULL);
		if (!input_context)
		{
			debug_fail("Failed to create XIM context");
			return;
		}

		unsigned long event_mask;
		char * const first_failed = XGetICValues(input_context, XNFilterEvents, &event_mask, NULL);
		if (first_failed)
		{
			debug_fail("Failed to get XIC masks");
			XDestroyIC(input_context);
			input_context = nullptr;
			return;
		}

		if (event_mask)
		{
			debug_printline("THERE IS A NEED FOR EVENT FILTERING");
			// Clients are expected to get the XIC value XNFilterEvents and augment
			// the event mask for the client window with that event mask. This mask
			// may be zero.
			// :point_up:
			// https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Input_Method_Overview
		}

		XSetICFocus(input_context);
	}
	void destroy_input_context()
	{
		if (input_context)
		{
			XUnsetICFocus(input_context);
			XDestroyIC(input_context);
		}
	}
}


namespace engine
{
	namespace application
	{
		namespace window
		{
			void create(const config_t & config)
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
					debug_printline(engine::application_channel, "glXQueryExtension: ", errorBase, " ", eventBase);
				}
				// glXQueryVersion
				{
					int major;
					int minor;

					if (!glXQueryVersion(render_display, &major, &minor))
					{
						throw std::runtime_error("glXQueryVersion: failed");
					}
					debug_printline(engine::application_channel, "glXQueryVersion: ", major, " ", minor);
				}
				// XDefaultScreen
				const int screen = XDefaultScreen(render_display);
#ifdef GLX_VERSION_1_1
				// glXGetClientString
				{
					debug_printline(engine::application_channel, "glXGetClientString GLX_VENDOR: ", glXGetClientString(render_display, GLX_VENDOR));
					debug_printline(engine::application_channel, "glXGetClientString GLX_VERSION: ", glXGetClientString(render_display, GLX_VERSION));
					// debug_printline(engine::application_channel, "glXGetClientString GLX_EXTENSIONS: ", glXGetClientString(render_display, GLX_EXTENSIONS));
				}
				// glXQueryServerString
				{
					debug_printline(engine::application_channel, "glXQueryServerString GLX_VENDOR: ", glXQueryServerString(render_display, screen, GLX_VENDOR));
					debug_printline(engine::application_channel, "glXQueryServerString GLX_VERSION: ", glXQueryServerString(render_display, screen, GLX_VERSION));
					// debug_printline(engine::application_channel, "glXQueryServerString GLX_EXTENSIONS: ", glXQueryServerString(render_display, screen, GLX_EXTENSIONS));
				}
				// glXQueryExtensionsString
				{
					// debug_printline(engine::application_channel, "glXQueryExtensionsString: ", glXQueryExtensionsString(render_display, screen));
				}
#endif

#ifdef GLX_VERSION_1_3
				std::vector<std::string> glx_extensions = utility::split(glXQueryExtensionsString(render_display, screen), ' ', true);

				// visual
				int fb_attributes[] = {
					GLX_X_RENDERABLE,  True,
					GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, // default
					GLX_DOUBLEBUFFER,  True,
					GLX_RED_SIZE,      8,
					GLX_GREEN_SIZE,    8,
					GLX_BLUE_SIZE,     8,
					GLX_ALPHA_SIZE,    8,
					GLX_DEPTH_SIZE,    24,
					GLX_STENCIL_SIZE,  8,
					GLX_RENDER_TYPE,   GLX_RGBA_BIT, // default
					GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, // default
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
				                                     config.window_width,
				                                     config.window_height,
				                                     0,
				                                     visual_info->depth,
				                                     InputOutput,
				                                     visual_info->visual,
				                                     CWColormap | CWEventMask,
				                                     &window_attributes);

				PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;
				if (std::count(std::begin(glx_extensions), std::end(glx_extensions), "GLX_ARB_create_context"))
				{
#ifdef GLX_VERSION_1_4
					glXCreateContextAttribsARB = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte *>("glXCreateContextAttribsARB")));
#else
					// TODO: check GLX_ARB_get_proc_address?
					glXCreateContextAttribsARB = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddressARB(reinterpret_cast<const GLubyte *>("glXCreateContextAttribsARB")));
#endif
				}
				GLXContext glx_context;
				if (glXCreateContextAttribsARB)
				{
					// glXUseXFont needs compatibility profile since 3.1
					const int attriblist[] = {
						GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
						GLX_CONTEXT_MINOR_VERSION_ARB, 0,
						// GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
						// GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
						0
					};
					glx_context = glXCreateContextAttribsARB(render_display,
					                                         fb_configs[0],
					                                         nullptr,
					                                         True,
					                                         attriblist);
				}
				else
				{
					glx_context = glXCreateNewContext(render_display,
					                                  fb_configs[0],
					                                  GLX_RGBA_TYPE,
					                                  nullptr,
					                                  True);
				}

				GLXWindow glx_window = glXCreateWindow(render_display,
				                                       fb_configs[0],
				                                       render_window,
				                                       nullptr);
#else
				// visual
				GLint visual_attributes[] = {
					GLX_RGBA,
					GLX_DOUBLEBUFFER,
					GLX_RED_SIZE, 8,
					GLX_GREEN_SIZE, 8,
					GLX_BLUE_SIZE, 8,
					GLX_ALPHA_SIZE, 8,
					GLX_DEPTH_SIZE, 24,
					GLX_STENCIL_SIZE, 8,
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

				create_input_method(*event_display);
				create_input_context(render_window);

				XSelectInput(event_display, render_window, ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | StructureNotifyMask);

				// XAutoRepeatOff(event_display); // dangerous!

				XMapWindow(render_display, render_window);
				//
				::event_display = event_display.detach();
				::render_display = render_display.detach();
				::render_window = render_window;
#ifdef GLX_VERSION_1_3
				::glx_window = glx_window;
				::glx_context = glx_context;
#else
				::render_context = render_context;
#endif
			}
			void destroy()
			{
				destroy_input_context();
				destroy_input_method();

#ifdef GLX_VERSION_1_3
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
#ifdef GLX_VERSION_1_3
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
								debug_printline(engine::application_channel, "wm_delete_window");

								XUnmapWindow(render_display, event.xclient.window);
							}
							break;
						default:
							debug_printline(engine::application_channel, "Event type(render_display): ", event.type);
						}
					}
				}
				{
					if (should_close_window.load(std::memory_order_relaxed))
					{
						XUnmapWindow(render_display, render_window);
					}
				}
				// swap buffers
#ifdef GLX_VERSION_1_3
				glXSwapBuffers(render_display, glx_window);
#else
				glXSwapBuffers(render_display, render_window);
#endif
			}

			XkbDescPtr load_key_names()
			{
				int lib_major = XkbMajorVersion;
				int lib_minor = XkbMinorVersion;

				if (XkbLibraryVersion(&lib_major, &lib_minor) == False)
				{
					debug_fail("Not compatible with runtime version of XKB");
					return nullptr;
				}

				int opcode;
				int event;
				int error;
				int major = XkbMajorVersion;
				int minor = XkbMinorVersion;

				if (XkbQueryExtension(event_display, &opcode, &event, &error, &major, &minor) == False)
				{
					debug_fail("Not compatible with server version of XKB");
					return nullptr;
				}

				XkbDescPtr const desc = XkbGetMap(event_display, 0, XkbUseCoreKbd);
				if (!desc)
				{
					debug_fail("Failed to get XKB map");
					return nullptr;
				}
				if (XkbGetNames(event_display, XkbKeyNamesMask, desc) != Success)
				{
					debug_fail("Failed to get XKB names");
					XkbFreeClientMap(desc, 0, True);
					return nullptr;
				}

				return desc;
			}
			void free_key_names(XkbDescPtr desc)
			{
				XkbFreeNames(desc, XkbKeyNamesMask, True);
				XkbFreeClientMap(desc, 0, True);
			}

#if TEXT_USE_X11
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
#endif

			int execute()
			{
				return messageLoop();
			}

			void close()
			{
				should_close_window.store(1, std::memory_order_relaxed);
			}
		}
	}
}

#endif /* WINDOW_USE_X11 */
