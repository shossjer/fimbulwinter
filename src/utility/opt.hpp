
#ifndef UTILITY_OPT_HPP
#define UTILITY_OPT_HPP

/**
 * Hint to the compiler that this path is never reached.
 */
#if defined(__GNUG__)
# define opt_unreachable() \
	__builtin_unreachable()
#elif defined(_MSC_VER)
# define opt_unreachable() \
	__assume(0)
#else
# define opt_unreachable() \
	std::terminate()
#endif

#endif /* UTILITY_OPT_HPP */
