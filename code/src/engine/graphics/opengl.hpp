
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
#if WINDOW_USE_USER32
	// 1.3
	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
#endif

	// 2.0
	extern PFNGLATTACHSHADERPROC glAttachShader;
	extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
	extern PFNGLCOMPILESHADERPROC glCompileShader;
	extern PFNGLCREATEPROGRAMPROC glCreateProgram;
	extern PFNGLCREATESHADERPROC glCreateShader;
	extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
	extern PFNGLDELETESHADERPROC glDeleteShader;
	extern PFNGLDETACHSHADERPROC glDetachShader;
	extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
	extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
	extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	extern PFNGLGETSHADERIVPROC glGetShaderiv;
	extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	extern PFNGLLINKPROGRAMPROC glLinkProgram;
	extern PFNGLSHADERSOURCEPROC glShaderSource;
	extern PFNGLUNIFORM1FPROC glUniform1f;
	extern PFNGLUNIFORM1IPROC glUniform1i;
	extern PFNGLUNIFORM2FPROC glUniform2f;
	extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	extern PFNGLUSEPROGRAMPROC glUseProgram;
	extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
	extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
	extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
	extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

	// 3.0
	extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
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
