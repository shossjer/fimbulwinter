#include "config.h"

#if WINDOW_USE_X11

#include "window.hpp"

#include "engine/application/config.hpp"
#include "engine/debug.hpp"

#include "utility/string.hpp"
#include "utility/unicode.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <poll.h>
#include <unistd.h>

#include <stdexcept>

namespace engine
{
	namespace graphics
	{
		extern void notify_resize(viewer & viewer, int width, int height);
	}

	namespace hid
	{
		extern void key_character(devices & devices, XKeyEvent & event, utility::unicode_code_point cp);

		extern void button_press(devices & devices, XButtonEvent & event);
		extern void button_release(devices & devices, XButtonEvent & event);
		extern void key_press(devices & devices, XKeyEvent & event);
		extern void key_release(devices & devices, XKeyEvent & event);
		extern void motion_notify(devices & devices, int x, int y, ::Time time);

		extern void notify_resize(ui & ui, int width, int height);
	}
}

namespace
{
	struct init_x11_threads_t
	{
		init_x11_threads_t()
		{
			XInitThreads();
		}
	} init_x11_threads;

	engine::graphics::viewer * viewer = nullptr;
	engine::hid::devices * devices = nullptr;
	engine::hid::ui * ui = nullptr;

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

	Display * display = nullptr;

	Window window;
#ifdef GLX_VERSION_1_3
	GLXWindow glx_window;
	GLXContext glx_context;
#else
	GLXContext glx_context;
#endif

	XIM input_method = nullptr;
	XIC input_context = nullptr;

	Atom wm_delete_window;

	utility::unicode_code_point get_unicode(XKeyEvent & event)
	{
		utility::unicode_code_point cp(0);
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
				cp = utility::unicode_code_point(buffer);
			}
			// debug_printline(cp);
		}
#else
# warning "Xutf8LookupString not supported, all unicode key events will be null"
		// try use XmbLookupString?
#endif
		return cp;
	}

	inline int message_loop()
	{
		// inspired by the works of the glfw peeps, they seem to know
		// their stuff :sunglasses:
		//
		// https://github.com/glfw/glfw/blob/a3d28ef52cec2fb69941bbce8a7ed7a2a22a8c41/src/x11_window.c#L2736
		//
		// after a lot of rewrites on how to separate the graphics from
		// the event loop, it seemed like the best approach might be the
		// simplest one of using `XInitThreads` and pray that it makes
		// X11 "thread safe" (there seem to be a lot of confusion about
		// this on the internet and the X11 documentation is rather thin
		// on the subject). The function `_glfwPlatformWaitEvents` was
		// the final piece of the puzzle, because without the insight of
		// why `XNextEvent` is no good there was this annoying block in
		// the graphics thread where the graphics would freeze unless
		// you constantly moved the mouse or window :joy:
		//
		// at one point in time there was a comment in the glfw source
		// code that explained this perfectly:
		//
		// select(1) is used instead of an X function like XNextEvent, as the
		// wait inside those are guarded by the mutex protecting the display
		// struct, locking out other threads from using X (including GLX)

		struct pollfd fds[1] = {
			{ConnectionNumber(::display), POLLIN, 0}
		};

		while (true)
		{
			const auto ret = poll(fds, 1, -1);
			if (ret == -1)
			{
				if (!debug_verify(errno == EINTR, "poll failed with ", errno))
					return -1;

				continue;
			}

			// removes any event we do not care about, maybe :thinking:
			if (XPending(::display) == 0)
				continue;

			while (XQLength(::display) != 0)
			{
				XEvent event;
				if (!debug_verify(XNextEvent(::display, &event) == 0, "XNextEvent failed"))
					return -1;

				if (XFilterEvent(&event, None) == True)
				{
					debug_printline(engine::application_channel, "filtererd event type ", event.type);

					continue;
				}

				switch (event.type)
				{
				case ButtonPress:
					button_press(*devices, event.xbutton);
					break;
				case ButtonRelease:
					button_release(*devices, event.xbutton);
					break;
				case ClientMessage:
					if ((Atom)event.xclient.data.l[0] == wm_delete_window)
					{
						debug_printline(engine::application_channel, "wm_delete_window");

						XUnmapWindow(display, event.xclient.window);
					}
					break;
				case ConfigureNotify:
					break;
					// case DestroyNotify:
					// 	break;
				case Expose:
					notify_resize(*::viewer, event.xexpose.width, event.xexpose.height);
					notify_resize(*::ui, event.xexpose.width, event.xexpose.height);
					break;
				case KeyPress:
					key_press(*devices, event.xkey);
					key_character(*devices, event.xkey, get_unicode(event.xkey));
					break;
				case KeyRelease:
					key_release(*devices, event.xkey);
					break;
				case MapNotify:
					break;
				case MotionNotify:
					motion_notify(*::devices, event.xmotion.x, event.xmotion.y, event.xmotion.time);
					break;
				case ReparentNotify:
					break;
				case UnmapNotify:
					return 0;
				default:
					debug_printline(engine::application_channel, "unhandled event type ", event.type);
				}
			}
		}
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

			input_method = nullptr;
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

			input_context = nullptr;
		}
	}
}


namespace engine
{
	namespace application
	{
		window::~window()
		{
			debug_assert(::display);

			destroy_input_context();
			destroy_input_method();

#ifdef GLX_VERSION_1_3
			glXDestroyWindow(::display, ::glx_window);
			glXDestroyContext(::display, ::glx_context);
			XDestroyWindow(::display, ::window);
			XCloseDisplay(::display);
#else
			glXDestroyContext(::display, ::glx_context);
			XDestroyWindow(::display, ::window);
			XCloseDisplay(::display);
#endif

			::display = nullptr;

			::ui = nullptr;
			::devices = nullptr;
			::viewer = nullptr;
		}

		window::window(const config_t & config)
		{
			debug_assert(!::display);

			// XOpenDisplay
			Display_guard display(nullptr);

			// glXQueryExtension
			{
				int errorBase;
				int eventBase;

				if (!glXQueryExtension(display, &errorBase, &eventBase))
				{
					throw std::runtime_error("glXQueryExtension: failed");
				}
				debug_printline(engine::application_channel, "glXQueryExtension: ", errorBase, " ", eventBase);
			}
			// glXQueryVersion
			{
				int major;
				int minor;

				if (!glXQueryVersion(display, &major, &minor))
				{
					throw std::runtime_error("glXQueryVersion: failed");
				}
				debug_printline(engine::application_channel, "glXQueryVersion: ", major, " ", minor);
			}
			// XDefaultScreen
			const int screen = XDefaultScreen(display);
#ifdef GLX_VERSION_1_1
			// glXGetClientString
			{
				debug_printline(engine::application_channel, "glXGetClientString GLX_VENDOR: ", glXGetClientString(display, GLX_VENDOR));
				debug_printline(engine::application_channel, "glXGetClientString GLX_VERSION: ", glXGetClientString(display, GLX_VERSION));
				// debug_printline(engine::application_channel, "glXGetClientString GLX_EXTENSIONS: ", glXGetClientString(display, GLX_EXTENSIONS));
			}
			// glXQueryServerString
			{
				debug_printline(engine::application_channel, "glXQueryServerString GLX_VENDOR: ", glXQueryServerString(display, screen, GLX_VENDOR));
				debug_printline(engine::application_channel, "glXQueryServerString GLX_VERSION: ", glXQueryServerString(display, screen, GLX_VERSION));
				// debug_printline(engine::application_channel, "glXQueryServerString GLX_EXTENSIONS: ", glXQueryServerString(display, screen, GLX_EXTENSIONS));
			}
			// glXQueryExtensionsString
			{
				// debug_printline(engine::application_channel, "glXQueryExtensionsString: ", glXQueryExtensionsString(display, screen));
			}
#endif

#ifdef GLX_VERSION_1_3
			std::vector<std::string> glx_extensions = utility::split(glXQueryExtensionsString(display, screen), ' ', true);

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

			GLXFBConfig_guard fb_configs(display, screen, fb_attributes, &n_buffers);
			XVisualInfo_guard visual_info(display, fb_configs[0]);
			// root
			Window root = XRootWindow(display, visual_info->screen);

			XSetWindowAttributes window_attributes;
			{
				window_attributes.colormap = XCreateColormap(display,
				                                             root,
				                                             visual_info->visual,
				                                             AllocNone);
				window_attributes.event_mask = NoEventMask;
			}
			Window window = XCreateWindow(display,
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
				glx_context = glXCreateContextAttribsARB(display,
				                                         fb_configs[0],
				                                         nullptr,
				                                         True,
				                                         attriblist);
			}
			else
			{
				glx_context = glXCreateNewContext(display,
				                                  fb_configs[0],
				                                  GLX_RGBA_TYPE,
				                                  nullptr,
				                                  True);
			}

			GLXWindow glx_window = glXCreateWindow(display,
			                                       fb_configs[0],
			                                       window,
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

			XVisualInfo_guard visual_info(display, 0, visual_attributes);
			// root
			Window root = XRootWindow(display, screen);

			XSetWindowAttributes window_attributes;
			{
				window_attributes.colormap = XCreateColormap(display,
				                                             root,
				                                             visual_info->visual,
				                                             AllocNone);
				window_attributes.event_mask = NoEventMask;
			}
			Window window = XCreateWindow(display,
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

			GLXContext glx_context = glXCreateContext(display,
			                                          visual_info,
			                                          nullptr,
			                                          True);
#endif
			wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
			{
				XSetWMProtocols(display, window, &wm_delete_window, 1);
			}

			create_input_method(*display);
			create_input_context(window);

			XSelectInput(display, window, ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | StructureNotifyMask);

			// XAutoRepeatOff(display); // dangerous!

			XMapWindow(display, window);

			::display = display.detach();
			::window = window;
#ifdef GLX_VERSION_1_3
			::glx_window = glx_window;
#endif
			::glx_context = glx_context;
		}

		void window::set_dependencies(engine::graphics::viewer & viewer, engine::hid::devices & devices, engine::hid::ui & ui)
		{
			::viewer = &viewer;
			::devices = &devices;
			::ui = &ui;
		}

		void make_current(window &)
		{
#ifdef GLX_VERSION_1_3
			debug_verify(glXMakeContextCurrent(display, glx_window, glx_window, glx_context) == True);
#else
			debug_verify(glXMakeCurrent(display, window, glx_context) == True);
#endif
		}

		void swap_buffers(window &)
		{
#ifdef GLX_VERSION_1_3
			glXSwapBuffers(display, glx_window);
#else
			glXSwapBuffers(display, window);
#endif
		}

		XkbDescPtr load_key_names(window &)
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

			if (XkbQueryExtension(display, &opcode, &event, &error, &major, &minor) == False)
			{
				debug_fail("Not compatible with server version of XKB");
				return nullptr;
			}

			XkbDescPtr const desc = XkbGetMap(display, 0, XkbUseCoreKbd);
			if (!desc)
			{
				debug_fail("Failed to get XKB map");
				return nullptr;
			}
			if (XkbGetNames(display, XkbKeyNamesMask, desc) != Success)
			{
				debug_fail("Failed to get XKB names");
				XkbFreeClientMap(desc, 0, True);
				return nullptr;
			}

			return desc;
		}

		void free_key_names(window &, XkbDescPtr desc)
		{
			XkbFreeNames(desc, XkbKeyNamesMask, True);
			XkbFreeClientMap(desc, 0, True);
		}

#if TEXT_USE_X11
		void buildFont(window & window, XFontStruct * font_struct, unsigned int first, unsigned int last, int base)
		{
			glXUseXFont(font_struct->fid, first, last - first + 1, base + first);
		}

		void freeFont(window & window, XFontStruct * font_struct)
		{
			XFreeFont(display, font_struct);
		}

		XFontStruct * loadFont(window & window, utility::string_view_utf8 name, int height)
		{
			int n_old_paths;
			char ** const old_paths = XGetFontPath(display, &n_old_paths);

			char * const pwd = get_current_dir_name();
			{
				const auto directory = utility::concat(pwd, "/res/font");

				const char * const new_paths[] = {directory.c_str()};
				int n_new_paths = 1;
				XSetFontPath(display, const_cast<char **>(new_paths), n_new_paths);
			}
			free(pwd);

			const auto format = utility::concat("-*-", name, "-*-*-*-*-*-*-*-", height * 6, "-*-*-iso8859-1");

			XFontStruct * const font_struct = XLoadQueryFont(display, format.c_str());

			XSetFontPath(display, old_paths, n_old_paths);

			XFreeFontPath(old_paths);

			return font_struct;
		}
#endif

		int execute(window &)
		{
			return message_loop();
		}

		void close()
		{
			XUnmapWindow(::display, ::window);
		}
	}
}

#endif /* WINDOW_USE_X11 */
