
#ifndef ENGINE_GRAPHICS_OPENGL_HPP
#define ENGINE_GRAPHICS_OPENGL_HPP

#include <config.h>

#if WINDOW_USE_USER32
// windows needs its header before any opengl header
# include <Windows.h>
# include <GL/gl.h>
// # include <GL/glext.h>
#elif WINDOW_USE_X11
# include <GL/gl.h>
# include <GL/glx.h>
#endif

#endif /* ENGINE_GRAPHICS_OPENGL_HPP */
