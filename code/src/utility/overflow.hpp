#pragma once

#if defined(_MSC_VER)
# include <intrin.h>
#endif

namespace utility
{
#if defined(_MSC_VER)
# if defined(_M_ARM)

#  error "these functions are completely untested!"

	inline bool add_overflow(signed char a, signed char b, signed char * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned char c = static_cast<unsigned char>(a) + static_cast<unsigned char>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 7) != 0;
	}

	inline bool add_overflow(short a, short b, short * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned short c = static_cast<unsigned short>(a) + static_cast<unsigned short>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 15) != 0;
	}

	inline bool add_overflow(int a, int b, int * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned int c = static_cast<unsigned int>(a) + static_cast<unsigned int>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 31) != 0;
	}

	inline bool add_overflow(long a, long b, long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned long c = static_cast<unsigned long>(a) + static_cast<unsigned long>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 31) != 0;
	}

	inline bool add_overflow(long long a, long long b, long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned long long c = static_cast<unsigned long long>(a) + static_cast<unsigned long long>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 63) != 0;
	}

	inline bool add_overflow(unsigned char a, unsigned char b, unsigned char * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned char c = a + b;

		*result = c;

		return c < a;
	}

	inline bool add_overflow(unsigned short a, unsigned short b, unsigned short * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned short c = a + b;

		*result = c;

		return c < a;
	}

	inline bool add_overflow(unsigned int a, unsigned int b, unsigned int * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned int c = a + b;

		*result = c;

		return c < a;
	}

	inline bool add_overflow(unsigned long a, unsigned long b, unsigned long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned long c = a + b;

		*result = c;

		return c < a;
	}

	inline bool add_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned long long c = a + b;

		*result = c;

		return c < a;
	}

# else

	inline bool add_overflow(signed char a, signed char b, signed char * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 30)
		unsigned char unsigned_result;
		const unsigned char carry = _addcarry_u8(0, (a ^ 0x80), (b ^ 0x80), &unsigned_result);

		*result = unsigned_result;

		return carry == (unsigned_result >> 7);
	}

	inline bool add_overflow(short a, short b, short * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 30)
		unsigned short unsigned_result;
		const unsigned char carry = _addcarry_u16(0, (a ^ 0x8000), (b ^ 0x8000), &unsigned_result);

		*result = unsigned_result;

		return carry == (unsigned_result >> 15);
	}

	inline bool add_overflow(int a, int b, int * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 30)
		unsigned int unsigned_result;
		const unsigned char carry = _addcarry_u32(0, (a ^ 0x80000000), (b ^ 0x80000000), &unsigned_result);

		*result = unsigned_result;

		return carry == (unsigned_result >> 31);
	}

	inline bool add_overflow(long a, long b, long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 30)
		unsigned int unsigned_result;
		const unsigned char carry = _addcarry_u32(0, (static_cast<int>(a) ^ 0x80000000), (static_cast<int>(b) ^ 0x80000000), &unsigned_result);

		*result = unsigned_result;

		return carry == (unsigned_result >> 31);
	}

#  if defined(_M_X64)

	inline bool add_overflow(long long a, long long b, long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 30)
		unsigned long long unsigned_result;
		const unsigned char carry = _addcarry_u64(0, (a ^ 0x8000000000000000), (b ^ 0x8000000000000000), &unsigned_result);

		*result = unsigned_result;

		return carry == (unsigned_result >> 63);
	}

#  else

	inline bool add_overflow(long long a, long long b, long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 29)
		const unsigned long long c = static_cast<unsigned long long>(a) + static_cast<unsigned long long>(b);

		*result = c;

		return (((~(a ^ b)) & (c ^ a)) >> 63) != 0;
	}

#  endif

	inline bool add_overflow(unsigned char a, unsigned char b, unsigned char * result)
	{
		return _addcarry_u8(0, a, b, result) != 0;
	}

	inline bool add_overflow(unsigned short a, unsigned short b, unsigned short * result)
	{
		return _addcarry_u16(0, a, b, result) != 0;
	}

	inline bool add_overflow(unsigned int a, unsigned int b, unsigned int * result)
	{
		return _addcarry_u32(0, a, b, result) != 0;
	}

	inline bool add_overflow(unsigned long a, unsigned long b, unsigned long * result)
	{
		return _addcarry_u32(0, static_cast<unsigned int>(a), static_cast<unsigned int>(b), reinterpret_cast<unsigned int *>(result)) != 0;
	}

#  if defined(_M_X64)

	inline bool add_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		return _addcarry_u64(0, a, b, result) != 0;
	}

#  else

	inline bool add_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 31)
		const unsigned long long c = a + b;

		*result = c;

		return c < a;
	}

#  endif

# endif

	inline bool mul_overflow(signed char a, signed char b, signed char * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		const short c = static_cast<short>(a) * static_cast<short>(b);
		const signed char ch = c >> 8;
		const signed char cl = static_cast<signed char>(c);

		*result = cl;

		return ch != (cl >> 7);
	}

	inline bool mul_overflow(short a, short b, short * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		const int c = static_cast<int>(a) * static_cast<int>(b);
		const short ch = c >> 16;
		const short cl = static_cast<short>(c);

		*result = cl;

		return ch != (cl >> 15);
	}

	inline bool mul_overflow(int a, int b, int * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		const long long c = static_cast<long long>(a) * static_cast<long long>(b);
		const int ch = c >> 32;
		const int cl = static_cast<int>(c);

		*result = cl;

		return ch != (cl >> 31);
	}

	inline bool mul_overflow(long a, long b, long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		const long long c = static_cast<long long>(a) * static_cast<long long>(b);
		const long ch = c >> 32;
		const long cl = static_cast<long>(c);

		*result = cl;

		return ch != (cl >> 31);
	}

# if defined(_M_X64)

	inline bool mul_overflow(long long a, long long b, long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		long long ch;
		const long long cl = _mul128(a, b, &ch);

		*result = cl;

		return ch != (cl >> 63);
	}

# elif defined(_M_ARM64)

	inline bool mul_overflow(long long a, long long b, long long * result)
	{
		// Hacker's Delight Second Edition, Henry S. Warren, Jr. (p. 32)
		const long long ch = __mulh(a, b);
		const long long cl = static_cast<unsigned long long>(a) * static_cast<unsigned long long>(b);

		*result = cl;

		return ch != (cl >> 63);
	}

# else

	inline bool mul_overflow(long long a, long long b, long long * result)
	{
		const unsigned long long al = static_cast<unsigned long long>(a) & 0xffffffffu;
		const unsigned long long ah = static_cast<unsigned long long>(a) >> 32;
		const unsigned long long bl = static_cast<unsigned long long>(b) & 0xffffffffu;
		const unsigned long long bh = static_cast<unsigned long long>(b) >> 32;

		const unsigned long long x = al * bl;
		const unsigned long long y = ah * bl;
		const unsigned long long z = al * bh;
		const unsigned long long w = ah * bh;

		const unsigned long long carry_sum = (x >> 32) + (y & 0xffffffff) + (z & 0xffffffff);

		const long long ch = (carry_sum >> 32) + (y >> 32) + (z >> 32) + w;
		const long long cl = (carry_sum << 32) | (x & 0xffffffff);

		*result = cl;

		return ch != (cl >> 63);
	}

#endif

	inline bool mul_overflow(unsigned char a, unsigned char b, unsigned char * result)
	{
		const unsigned short c = static_cast<unsigned short>(a) * static_cast<unsigned short>(b);
		const unsigned char ch = c >> 8;
		const unsigned char cl = static_cast<unsigned char>(c);

		*result = cl;

		return ch != 0;
	}

	inline bool mul_overflow(unsigned short a, unsigned short b, unsigned short * result)
	{
		const unsigned int c = static_cast<unsigned int>(a) * static_cast<unsigned int>(b);
		const unsigned short ch = c >> 16;
		const unsigned short cl = static_cast<unsigned short>(c);

		*result = cl;

		return ch != 0;
	}

	inline bool mul_overflow(unsigned int a, unsigned int b, unsigned int * result)
	{
		const unsigned long long c = static_cast<unsigned long long>(a) * static_cast<unsigned long long>(b);
		const unsigned int ch = c >> 32;
		const unsigned int cl = static_cast<unsigned int>(c);

		*result = cl;

		return ch != 0;
	}

	inline bool mul_overflow(unsigned long a, unsigned long b, unsigned long * result)
	{
		const unsigned long long c = static_cast<unsigned long long>(a) * static_cast<unsigned long long>(b);
		const unsigned long ch = c >> 32;
		const unsigned long cl = static_cast<unsigned long>(c);

		*result = cl;

		return ch != 0;
	}

# if defined(_M_X64)

	inline bool mul_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		unsigned long long ch;
		const unsigned long long cl = _umul128(a, b, &ch);

		*result = cl;

		return ch != 0;
	}

# elif defined(_M_ARM64)

	inline bool mul_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		const unsigned long long ch = __umulh(a, b);
		const unsigned long long cl = a * b;

		*result = cl;

		return ch != 0;
	}

# else

	inline bool mul_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		const unsigned long long al = a & 0xffffffff;
		const unsigned long long ah = a >> 32;
		const unsigned long long bl = b & 0xffffffff;
		const unsigned long long bh = b >> 32;

		const unsigned long long x = al * bl;
		const unsigned long long y = ah * bl;
		const unsigned long long z = al * bh;
		const unsigned long long w = ah * bh;

		const unsigned long long carry_sum = (x >> 32) + (y & 0xffffffff) + (z & 0xffffffff);

		const unsigned long long ch = (carry_sum >> 32) + (y >> 32) + (z >> 32) + w;
		const unsigned long long cl = (carry_sum << 32) | (x & 0xffffffff);

		*result = cl;

		return ch != 0;
	}

#endif

#elif defined(__GNUG__)

	inline bool add_overflow(signed char a, signed char b, signed char * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(short a, short b, short * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(int a, int b, int * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(long a, long b, long * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(long long a, long long b, long long * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(unsigned char a, unsigned char b, unsigned char * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(unsigned short a, unsigned short b, unsigned short * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(unsigned int a, unsigned int b, unsigned int * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(unsigned long a, unsigned long b, unsigned long * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool add_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		return __builtin_add_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(signed char a, signed char b, signed char * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(short a, short b, short * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(int a, int b, int * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(long a, long b, long * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(long long a, long long b, long long * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(unsigned char a, unsigned char b, unsigned char * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(unsigned short a, unsigned short b, unsigned short * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(unsigned int a, unsigned int b, unsigned int * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(unsigned long a, unsigned long b, unsigned long * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

	inline bool mul_overflow(unsigned long long a, unsigned long long b, unsigned long long * result)
	{
		return __builtin_mul_overflow(a, b, result) != 0;
	}

#endif
}
