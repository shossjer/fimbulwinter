#pragma once

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_expect) || defined(__GNUC__)
// optimize knowing that this branch is almost always the one
# define fiw_likely(x) __builtin_expect(!!(x), 1)
#else
// optimize knowing that this branch is almost always the one
# define fiw_likely(x) !!(x)
#endif

#if __has_builtin(__builtin_unreachable) ||\
	(defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 5))))
// optimize knowing that this branch is impossible
# define fiw_unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
// optimize knowing that this branch is impossible
# define fiw_unreachable() __assume(0)
#else
# error Missing implementation!
#endif

// silence warning about unused variable
# define fiw_unused(x) static_cast<void>(x)
