
#include "opengl.hpp"

namespace engine
{
namespace graphics
{
namespace opengl
{
	// 2.0
	PFNGLATTACHSHADERPROC glAttachShader = nullptr;
	PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = nullptr;
	PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
	PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
	PFNGLCREATESHADERPROC glCreateShader = nullptr;
	PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
	PFNGLDELETESHADERPROC glDeleteShader = nullptr;
	PFNGLDETACHSHADERPROC glDetachShader = nullptr;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
	PFNGLDRAWBUFFERSPROC glDrawBuffers = nullptr;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
	PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
	PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
	PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
	PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
	PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
	PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
	PFNGLUNIFORM1FPROC glUniform1f = nullptr;
	PFNGLUNIFORM1IPROC glUniform1i = nullptr;
	PFNGLUNIFORM2FPROC glUniform2f = nullptr;
	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
	PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
	PFNGLVALIDATEPROGRAMPROC glValidateProgram = nullptr;
	PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = nullptr;
	PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = nullptr;
	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;

	// 3.0
	PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = nullptr;
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
		glAttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(wglGetProcAddress("glAttachShader"));
		glBindAttribLocation = reinterpret_cast<PFNGLBINDATTRIBLOCATIONPROC>(wglGetProcAddress("glBindAttribLocation"));
		glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(wglGetProcAddress("glCompileShader"));
		glCreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(wglGetProcAddress("glCreateProgram"));
		glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(wglGetProcAddress("glCreateShader"));
		glDeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(wglGetProcAddress("glDeleteProgram"));
		glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(wglGetProcAddress("glDeleteShader"));
		glDetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(wglGetProcAddress("glDetachShader"));
		glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(wglGetProcAddress("glDisableVertexAttribArray"));
		glDrawBuffers = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(wglGetProcAddress("glDrawBuffers"));
		glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(wglGetProcAddress("glEnableVertexAttribArray"));
		glFramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(wglGetProcAddress("glFramebufferTexture2D"));
		glGetAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(wglGetProcAddress("glGetAttriblocation"));
		glGetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(wglGetProcAddress("glGetProgramiv"));
		glGetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(wglGetProcAddress("glGetProgramInfoLog"));
		glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(wglGetProcAddress("glGetShaderiv"));
		glGetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(wglGetProcAddress("glGetShaderInfoLog"));
		glGetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(wglGetProcAddress("glGetUniformLocation"));
		glLinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(wglGetProcAddress("glLinkProgram"));
		glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(wglGetProcAddress("glShaderSource"));
		glUniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(wglGetProcAddress("glUniform1f"));
		glUniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(wglGetProcAddress("glUniform1i"));
		glUniform2f = reinterpret_cast<PFNGLUNIFORM2FPROC>(wglGetProcAddress("glUniform2f"));
		glUniformMatrix4fv = reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(wglGetProcAddress("glUniformMatrix4fv"));
		glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(wglGetProcAddress("glUseProgram"));
		glValidateProgram = reinterpret_cast<PFNGLVALIDATEPROGRAMPROC>(wglGetProcAddress("glValidateProgram"));
		glVertexAttrib4f = reinterpret_cast<PFNGLVERTEXATTRIB4FPROC>(wglGetProcAddress("glVertexAttrib4f"));
		glVertexAttrib4fv = reinterpret_cast<PFNGLVERTEXATTRIB4FVPROC>(wglGetProcAddress("glVertexAttrib4fv"));
		glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(wglGetProcAddress("glVertexAttribPointer"));

		glBindFragDataLocation = reinterpret_cast<PFNGLBINDFRAGDATALOCATIONPROC>(wglGetProcAddress("glBindFragDataLocation"));
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
		glAttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glAttachShader")));
		glBindAttribLocation = reinterpret_cast<PFNGLBINDATTRIBLOCATIONPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBindAttribLocation")));
		glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glCompileShader")));
		glCreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glCreateProgram")));
		glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glCreateShader")));
		glDeleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteProgram")));
		glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteShader")));
		glDetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDetachShader")));
		glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDisableVertexAttribArray")));
		glDrawBuffers = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDrawBuffers")));
		glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glEnableVertexAttribArray")));
		glFramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glFramebufferTexture2D")));
		glGetAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetAttriblocation")));
		glGetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetProgramiv")));
		glGetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetProgramInfoLog")));
		glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetShaderiv")));
		glGetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetShaderInfoLog")));
		glGetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGetUniformLocation")));
		glLinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glLinkProgram")));
		glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glShaderSource")));
		glUniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glUniform1f")));
		glUniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glUniform1i")));
		glUniform2f = reinterpret_cast<PFNGLUNIFORM2FPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glUniform2f")));
		glUniformMatrix4fv = reinterpret_cast<PFNGLUNIFORMMATRIX4FVPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glUniformMatrix4fv")));
		glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glUseProgram")));
		glValidateProgram = reinterpret_cast<PFNGLVALIDATEPROGRAMPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glValidateProgram")));
		glVertexAttrib4f = reinterpret_cast<PFNGLVERTEXATTRIB4FPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glVertexAttrib4f")));
		glVertexAttrib4fv = reinterpret_cast<PFNGLVERTEXATTRIB4FVPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glVertexAttrib4fv")));
		glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glVertexAttribPointer")));

		glBindFragDataLocation = reinterpret_cast<PFNGLBINDFRAGDATALOCATIONPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBindFragDataLocation")));
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
