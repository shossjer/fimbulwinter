
#ifndef UTILITY_INTRINSICS_HPP
#define UTILITY_INTRINSICS_HPP

#if defined(_MSC_VER)
# include <intrin.h>
#endif

/**
 * Hint to the compiler that this path will never be reached.
 */
#if defined(__GNUG__)
# define intrinsic_unreachable() __builtin_unreachable()
#elif defined (_MSC_VER)
# define intrinsic_unreachable() __assume(0)
#else
# define intrinsic_unreachable() do {} while(0)
#endif

#endif /* UTILITY_INTRINSICS_HPP */
