
#include "window.hpp"

#include <config.h>

#if WINDOW_USE_USER32
# include "window_user32.cpp"
#elif WINDOW_USE_X11
# include "window_x11.cpp"
#endif
