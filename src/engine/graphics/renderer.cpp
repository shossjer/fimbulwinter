
#include "renderer.hpp"

#include <config.h>

#if THREAD_USE_KERNEL32
# include "renderer_kernel32.cpp"
#elif THREAD_USE_PTHREAD
# include "renderer_pthread.cpp"
#endif
