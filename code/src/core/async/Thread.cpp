
#include "Thread.hpp"

#include <config.h>

#if THREAD_USE_KERNEL32
# include "Thread_kernel32.cpp"
#elif THREAD_USE_PTHREAD
# include "Thread_pthread.cpp"
#endif
