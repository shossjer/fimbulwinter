#pragma once

#include "utility/intrinsics.hpp"
#include "utility/string.hpp"

#include <iostream>

namespace utility
{
	// unicode_points
	struct point_difference : Arithmetic<point_difference, std::ptrdiff_t>
	{
		using Arithmetic<point_difference, std::ptrdiff_t>::Arithmetic;
	};

	// unicode_units
	struct unit_difference : Arithmetic<unit_difference, std::ptrdiff_t>
	{
		using Arithmetic<unit_difference, std::ptrdiff_t>::Arithmetic;
	};

	// unicode_diff
	template <typename Encoding>
	class lazy_difference
	{
		using encoding_traits = encoding_traits<Encoding>;

		using code_unit = typename encoding_traits::code_unit;
	private:
		const code_unit * begin_;
		const code_unit * end_;

	public:
		constexpr lazy_difference(const code_unit * begin, const code_unit * end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr operator point_difference () const
		{
			if (end_ < begin_)
				return point_difference(-encoding_traits::count(end_, begin_));
			return point_difference(encoding_traits::count(begin_, end_));
		}
		constexpr operator unit_difference () const { return unit_difference(end_ - begin_); }
	};

	class unicode_code_point
	{
	private:
		uint32_t value_;

	public:
		unicode_code_point() = default;
		explicit constexpr unicode_code_point(int32_t value)
			: value_(value)
		{}
		explicit constexpr unicode_code_point(uint32_t value)
			: value_(value)
		{}
		explicit constexpr unicode_code_point(const char * s)
			: value_(extract_value(s, extract_size(s)))
		{}
		explicit constexpr unicode_code_point(const char16_t * s)
			: value_(extract_value(s, extract_size(s)))
		{}
		explicit constexpr unicode_code_point(const char32_t * s)
			: value_(*s)
		{}

	public:
		uint32_t value() const { return value_; }

		int get(char * s) const
		{
			if (value_ < 0x80)
			{
				s[0] = static_cast<char>(value_);
				return 1;
			}
			if (value_ < 0x800)
			{
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xc0 = 1100 0000
				s[0] = static_cast<char>((value_ >> 6) | 0xc0);
				s[1] = static_cast<char>((value_ & 0x3f) | 0x80);
				return 2;
			}
			if (value_ < 0x10000)
			{
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xe0 = 1110 0000
				s[0] = static_cast<char>((value_ >> 12) | 0xe0);
				s[1] = static_cast<char>(((value_ >> 6) & 0x3f) | 0x80);
				s[2] = static_cast<char>((value_ & 0x3f) | 0x80);
				return 3;
			}
			// else
			// {
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xf0 = 1111 0000
				s[0] = static_cast<char>((value_ >> 18) | 0xf0);
				s[1] = static_cast<char>(((value_ >> 12) & 0x3f) | 0x80);
				s[2] = static_cast<char>(((value_ >> 6) & 0x3f) | 0x80);
				s[3] = static_cast<char>((value_ & 0x3f) | 0x80);
				return 4;
			// }
		}
		int get(char16_t * s) const
		{
			if (value_ < 0x10000)
			{
				s[0] = static_cast<char16_t>(value_);
				return 1;
			}
			// else
			// {
				const auto value = value_ - 0x10000;
				// 0x03ff = 0000 0011 1111 1111
				// 0xd800 = 1101 1000 0000 0000
				// 0xdc00 = 1101 1100 0000 0000
				s[0] = static_cast<char16_t>((value >> 10) | 0xd800);
				s[1] = static_cast<char16_t>((value_ & 0x03ff) | 0xdc00);
				return 2;
			// }
		}
		int get(char32_t * s) const
		{
			s[0] = value_;
			return 1;
		}

	public:
		friend constexpr bool operator == (unicode_code_point a, unicode_code_point b) { return a.value_ == b.value_; }
		friend constexpr bool operator == (unicode_code_point a, uint32_t b) { return a.value_ == b; }
		friend constexpr bool operator == (uint32_t a, unicode_code_point b) { return a == b.value_; }
		friend constexpr bool operator != (unicode_code_point a, unicode_code_point b) { return !(a == b); }
		friend constexpr bool operator != (unicode_code_point a, uint32_t b) { return !(a == b); }
		friend constexpr bool operator != (uint32_t a, unicode_code_point b) { return !(a == b); }
		friend constexpr bool operator < (unicode_code_point a, unicode_code_point b) { return a.value_ < b.value_; }
		friend constexpr bool operator < (unicode_code_point a, uint32_t b) { return a.value_ < b; }
		friend constexpr bool operator < (uint32_t a, unicode_code_point b) { return a < b.value_; }
		friend constexpr bool operator <= (unicode_code_point a, unicode_code_point b) { return !(b < a); }
		friend constexpr bool operator <= (unicode_code_point a, uint32_t b) { return !(b < a); }
		friend constexpr bool operator <= (uint32_t a, unicode_code_point b) { return !(b < a); }
		friend constexpr bool operator > (unicode_code_point a, unicode_code_point b) { return b < a; }
		friend constexpr bool operator > (unicode_code_point a, uint32_t b) { return b < a; }
		friend constexpr bool operator > (uint32_t a, unicode_code_point b) { return b < a; }
		friend constexpr bool operator >= (unicode_code_point a, unicode_code_point b) { return !(a < b); }
		friend constexpr bool operator >= (unicode_code_point a, uint32_t b) { return !(a < b); }
		friend constexpr bool operator >= (uint32_t a, unicode_code_point b) { return !(a < b); }

		template <typename Traits>
		friend std::basic_ostream<char, Traits> & operator << (std::basic_ostream<char, Traits> & os, unicode_code_point x)
		{
			char chars[4];
			const int size = x.get(chars);

			return os.write(chars, size);
		}
		template <typename Traits>
		friend std::basic_ostream<char16_t, Traits> & operator << (std::basic_ostream<char16_t, Traits> & os, unicode_code_point x)
		{
			char16_t chars[2];
			const int size = x.get(chars);

			return os.write(chars, size);
		}
		template <typename Traits>
		friend std::basic_ostream<char32_t, Traits> & operator << (std::basic_ostream<char32_t, Traits> & os, unicode_code_point x)
		{
			char32_t chars[1];
			const int size = x.get(chars);

			return os.write(chars, size);
		}

	public:
		template <typename Char>
		static constexpr std::ptrdiff_t count_impl(const Char * from, const Char * to, std::ptrdiff_t length)
		{
			return from < to ? count_impl(from + next(from), to, length + 1) : length;
		}
		static constexpr std::ptrdiff_t count(const char * from, const char * to)
		{
			return count_impl(from, to, 0);
		}
		static constexpr std::ptrdiff_t count(const char16_t * from, const char16_t * to)
		{
			return count_impl(from, to, 0);
		}
		static constexpr std::ptrdiff_t count(const char32_t * from, const char32_t * to)
		{
			return to - from;
		}

		static constexpr std::ptrdiff_t next(const char * s)
		{
			return extract_size(s);
		}
		static constexpr std::ptrdiff_t next(const char16_t * s)
		{
			return extract_size(s);
		}
		static constexpr std::ptrdiff_t next(const char32_t *)
		{
			return 1;
		}

		static constexpr std::ptrdiff_t previous_impl(const char * s, const char * from)
		{
			// 0x80 = 1000 0000
			// 0xc0 = 1100 0000
			return (*s & 0xc0) == 0x80 ? previous_impl(s - 1, from) : from - s;
		}
		static constexpr std::ptrdiff_t previous(const char * s)
		{
			return previous_impl(s - 1, s);
		}

		static constexpr std::ptrdiff_t previous_impl(const char * s, std::ptrdiff_t length, const char * from)
		{
			return length <= 0 ? from - s : previous_impl(s - previous(s), length - 1, from);
		}
		static constexpr std::ptrdiff_t previous(const char *, utility::unit_difference length) { return length.get(); }
		static constexpr std::ptrdiff_t previous(const char * s, utility::point_difference length)
		{
			return previous_impl(s, length.get(), s);
		}
		template <typename Encoding>
		static constexpr std::ptrdiff_t previous(const char * s, utility::lazy_difference<Encoding> length)
		{
			return next(s, utility::unit_difference(length));
		}

		static constexpr std::ptrdiff_t next_impl(const char * s, std::ptrdiff_t length, const char * from)
		{
			return length <= 0 ? s - from : next_impl(s + next(s), length - 1, from);
		}
		static constexpr std::ptrdiff_t next(const char *, utility::unit_difference length) { return length.get(); }
		static constexpr std::ptrdiff_t next(const char * s, utility::point_difference length)
		{
			return next_impl(s, length.get(), s);
		}
		template <typename Encoding>
		static constexpr std::ptrdiff_t next(const char * s, utility::lazy_difference<Encoding> length)
		{
			return next(s, utility::unit_difference(length));
		}

		static constexpr int extract_size(const char * s)
		{
			constexpr int size_table[16] = {
				1, 1, 1, 1,
				1, 1, 1, 1,
				-1, -1, -1, -1,
				2, 2, 3, 4,
			};
			return size_table[uint8_t(s[0]) >> 4];
		}
		static constexpr int extract_size(const char16_t * s)
		{
			// 0xd800 = 1101 1000 0000 0000
			// 0xfc00 = 1111 1100 0000 0000
			return (s[0] & 0xfc00) == 0xd800 ? 2 : 1;
		}

		static constexpr uint32_t extract_value(const char * s, int size)
		{
			switch (size)
			{
				// 0x07 = 0000 0111
				// 0x3f = 0011 1111
			case 4: return uint32_t(s[0] & 0x07) << 18 | uint32_t(s[1] & 0x3f) << 12 | uint32_t(s[2] & 0x3f) << 6 | uint32_t(s[3] & 0x3f);
				// 0x0f = 0000 1111
				// 0x3f = 0011 1111
			case 3: return uint32_t(s[0] & 0x0f) << 12 | uint32_t(s[1] & 0x3f) << 6 | uint32_t(s[2] & 0x3f);
				// 0x1f = 0001 1111
				// 0x3f = 0011 1111
			case 2: return uint32_t(s[0] & 0x1f) << 6 | uint32_t(s[1] & 0x3f);
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}
		static constexpr uint32_t extract_value(const char16_t * s, int size)
		{
			switch (size)
			{
				// 0x03ff = 0000 0011 1111 1111
			case 2: return (uint32_t(s[0] & 0x03ff) << 10 | uint32_t(s[1] & 0x03ff)) + 0x10000;
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}
	};

	struct encoding_utf8
	{
		using code_unit = char;
		using code_point = utility::unicode_code_point;

		using difference_type = lazy_difference<encoding_utf8>;

		static constexpr std::size_t max_size() { return 4; } // todo remove?

		static difference_type difference(const code_unit * begin, const code_unit * end) { return {begin, end}; }
	};

	struct encoding_utf16
	{
		using code_unit = char16_t;
		using code_point = utility::unicode_code_point;

		using difference_type = lazy_difference<encoding_utf16>;

		static constexpr std::size_t max_size() { return 2; } // todo remove?

		static difference_type difference(const code_unit * begin, const code_unit * end) { return {begin, end}; }
	};

	struct encoding_utf32
	{
		using code_unit = char32_t;
		using code_point = utility::unicode_code_point;

		using difference_type = lazy_difference<encoding_utf32>;

		static constexpr std::size_t max_size() { return 1; } // todo remove?

		static difference_type difference(const code_unit * begin, const code_unit * end) { return {begin, end}; }
	};

	using string_view_utf8 = basic_string_view<utility::encoding_utf8>;

	using heap_string_utf8 = basic_string<utility::heap_storage_traits, encoding_utf8>;
	template <std::size_t Capacity>
	using static_string_utf8 = basic_string<utility::static_storage_traits<Capacity>, encoding_utf8>;
}
