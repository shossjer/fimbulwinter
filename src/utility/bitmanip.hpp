
#ifndef UTILITY_BITMANIP_HPP
#define UTILITY_BITMANIP_HPP

#include <cstdint>

namespace utility
{
	inline uint32_t clp2(uint32_t x)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr.
		x = x - 1;
		x = x | (x >> 1);
		x = x | (x >> 2);
		x = x | (x >> 4);
		x = x | (x >> 8);
		x = x | (x >> 16);
		return x + 1;
	}

	inline int ntz(uint64_t x)
	{
#ifdef __GNUG__
		if (!x)
			return 64;
		return __builtin_ctzll(x);
#else
		// Hacker's Delight Second Edition, Henry S. Warren, Jr.
		// Gaudet's algorithm
		const uint64_t y = x & -x;
		const int bz = y ? 0 : 1;
		const int b5 = (y & 0x00000000ffffffffull) ? 0 : 32;
		const int b4 = (y & 0x0000ffff0000ffffull) ? 0 : 16;
		const int b3 = (y & 0x00ff00ff00ff00ffull) ? 0 :  8;
		const int b2 = (y & 0x0f0f0f0f0f0f0f0full) ? 0 :  4;
		const int b1 = (y & 0x3333333333333333ull) ? 0 :  2;
		const int b0 = (y & 0x5555555555555555ull) ? 0 :  1;
		return bz + b5 + b4 + b3 + b2 + b1 + b0;
#endif
	}
}

#endif /* UTILITY_BITMANIP_HPP */
