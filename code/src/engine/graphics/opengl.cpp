
#include "opengl.hpp"

namespace engine
{
namespace graphics
{
namespace opengl
{
	// 3.0
	PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
	PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
	PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = nullptr;
	PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
	PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
	PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
	PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
	PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
	PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
	
	void init()
	{
#if WINDOW_USE_USER32
		glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(wglGetProcAddress("glBindFramebuffer"));
		glBindRenderbuffer = reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(wglGetProcAddress("glBindRenderbuffer"));
		glBlitFramebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(wglGetProcAddress("glBlitFramebuffer"));
		glDeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(wglGetProcAddress("glDeleteFramebuffers"));
		glDeleteRenderbuffers = reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(wglGetProcAddress("glDeleteRenderbuffers"));
		glFramebufferRenderbuffer = reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(wglGetProcAddress("glFramebufferRenderbuffer"));
		glGenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(wglGetProcAddress("glGenFramebuffers"));
		glGenRenderbuffers = reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(wglGetProcAddress("glGenRenderbuffers"));
		glRenderbufferStorage = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(wglGetProcAddress("glRenderbufferStorage"));
#elif WINDOW_USE_X11
		glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBindFramebuffer")));
		glBindRenderbuffer = reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBindRenderbuffer")));
		glBlitFramebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBlitFramebuffer")));
		glDeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteFramebuffers")));
		glDeleteRenderbuffers = reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteRenderbuffers")));
		glFramebufferRenderbuffer = reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glFramebufferRenderbuffer")));
		glGenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGenFramebuffers")));
		glGenRenderbuffers = reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGenRenderbuffers")));
		glRenderbufferStorage = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glRenderbufferStorage")));
#endif
	}
}
}
}
