
#ifndef ENGINE_GRAPHICS_OPENGL_HPP
#define ENGINE_GRAPHICS_OPENGL_HPP

#include <config.h>

#if WINDOW_USE_USER32
// windows needs its header before any opengl header
# include <windows.h>
# include <GL/gl.h>
# include <GL/glext.h>
#elif WINDOW_USE_X11
# include <GL/gl.h>
# include <GL/glx.h>
# include <GL/glext.h>
#endif

namespace engine
{
namespace graphics
{
namespace opengl
{
	// 3.0
	extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
	extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;

	void init();
}
}
}

#endif /* ENGINE_GRAPHICS_OPENGL_HPP */
