#pragma once

#include "utility/encoding_traits.hpp"
#include "utility/intrinsics.hpp"

#include <iostream>

namespace utility
{
	using unicode_code_unit_utf8 = char;
	using unicode_code_unit_utf16 = char16_t;
	using unicode_code_unit_utf32 = char32_t;
#if defined(_MSC_VER) && defined(_UNICODE)
	using unicode_code_unit_utfw = wchar_t;
#endif

	class unicode_code_point
	{
#if defined(_MSC_VER) && defined(_UNICODE)
		static_assert(sizeof(wchar_t) == sizeof(char16_t), "win32 should encode utf-16le with `wchar_t`");
#endif

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

		explicit constexpr unicode_code_point(const unicode_code_unit_utf8 * s)
			: value_(extract_value(s, extract_size(s)))
		{}

		explicit constexpr unicode_code_point(const unicode_code_unit_utf16 * s)
			: value_(extract_value(s, extract_size(s)))
		{}

		explicit constexpr unicode_code_point(const unicode_code_unit_utf32 * s)
			: value_(extract_value(s, extract_size(s)))
		{}

#if defined(_MSC_VER) && defined(_UNICODE)
		explicit constexpr unicode_code_point(const unicode_code_unit_utfw * s)
			: value_(extract_value(s, extract_size(s)))
		{}
#endif

	public:

		constexpr uint32_t value() const { return value_; }

		constexpr int get(unicode_code_unit_utf8 * s) const
		{
			if (value_ < 0x80)
			{
				s[0] = static_cast<unicode_code_unit_utf8>(value_);
				return 1;
			}
			if (value_ < 0x800)
			{
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xc0 = 1100 0000
				s[0] = static_cast<unicode_code_unit_utf8>((value_ >> 6) | 0xc0);
				s[1] = static_cast<unicode_code_unit_utf8>((value_ & 0x3f) | 0x80);
				return 2;
			}
			if (value_ < 0x10000)
			{
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xe0 = 1110 0000
				s[0] = static_cast<unicode_code_unit_utf8>((value_ >> 12) | 0xe0);
				s[1] = static_cast<unicode_code_unit_utf8>(((value_ >> 6) & 0x3f) | 0x80);
				s[2] = static_cast<unicode_code_unit_utf8>((value_ & 0x3f) | 0x80);
				return 3;
			}
			// else
			// {
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xf0 = 1111 0000
				s[0] = static_cast<unicode_code_unit_utf8>((value_ >> 18) | 0xf0);
				s[1] = static_cast<unicode_code_unit_utf8>(((value_ >> 12) & 0x3f) | 0x80);
				s[2] = static_cast<unicode_code_unit_utf8>(((value_ >> 6) & 0x3f) | 0x80);
				s[3] = static_cast<unicode_code_unit_utf8>((value_ & 0x3f) | 0x80);
				return 4;
			// }
		}

		constexpr int get(unicode_code_unit_utf16 * s) const
		{
			if (value_ < 0x10000)
			{
				s[0] = static_cast<unicode_code_unit_utf16>(value_);
				return 1;
			}
			// else
			// {
				const auto value = value_ - 0x10000;
				// 0x03ff = 0000 0011 1111 1111
				// 0xd800 = 1101 1000 0000 0000
				// 0xdc00 = 1101 1100 0000 0000
				s[0] = static_cast<unicode_code_unit_utf16>((value >> 10) | 0xd800);
				s[1] = static_cast<unicode_code_unit_utf16>((value_ & 0x03ff) | 0xdc00);
				return 2;
			// }
		}

		constexpr int get(unicode_code_unit_utf32 * s) const
		{
			s[0] = value_;
			return 1;
		}

#if defined(_MSC_VER) && defined(_UNICODE)
		constexpr int get(unicode_code_unit_utfw * s) const
		{
			if (value_ < 0x10000)
			{
				s[0] = static_cast<unicode_code_unit_utfw>(value_);
				return 1;
			}
			// else
			// {
				const auto value = value_ - 0x10000;
				// 0x03ff = 0000 0011 1111 1111
				// 0xd800 = 1101 1000 0000 0000
				// 0xdc00 = 1101 1100 0000 0000
				s[0] = static_cast<unicode_code_unit_utfw>((value >> 10) | 0xd800);
				s[1] = static_cast<unicode_code_unit_utfw>((value_ & 0x03ff) | 0xdc00);
				return 2;
			// }
		}
#endif

		static constexpr int extract_size(const unicode_code_unit_utf8 * s)
		{
			constexpr int size_table[16] = {
				1, 1, 1, 1,
				1, 1, 1, 1,
				-1, -1, -1, -1,
				2, 2, 3, 4,
			};
			return size_table[uint8_t(s[0]) >> 4];
		}

		static constexpr int extract_size(const unicode_code_unit_utf16 * s)
		{
			// 0xd800 = 1101 1000 0000 0000
			// 0xfc00 = 1111 1100 0000 0000
			return (s[0] & 0xfc00) == 0xd800 ? 2 : 1;
		}

		static constexpr int extract_size(const unicode_code_unit_utf32 * /*s*/) { return 1; }

#if defined(_MSC_VER) && defined(_UNICODE)
		static constexpr int extract_size(const unicode_code_unit_utfw * s)
		{
			// 0xd800 = 1101 1000 0000 0000
			// 0xfc00 = 1111 1100 0000 0000
			return (s[0] & 0xfc00) == 0xd800 ? 2 : 1;
		}
#endif

		static constexpr uint32_t extract_value(const unicode_code_unit_utf8 * s, int size)
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

		static constexpr uint32_t extract_value(const unicode_code_unit_utf16 * s, int size)
		{
			switch (size)
			{
				// 0x03ff = 0000 0011 1111 1111
			case 2: return (uint32_t(s[0] & 0x03ff) << 10 | uint32_t(s[1] & 0x03ff)) + 0x10000;
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}

		static constexpr uint32_t extract_value(const unicode_code_unit_utf32 * s, int /*size*/) { return s[0]; }

#if defined(_MSC_VER) && defined(_UNICODE)
		static constexpr uint32_t extract_value(const unicode_code_unit_utfw * s, int size)
		{
			switch (size)
			{
				// 0x03ff = 0000 0011 1111 1111
			case 2: return (uint32_t(s[0] & 0x03ff) << 10 | uint32_t(s[1] & 0x03ff)) + 0x10000;
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}
#endif

	private:

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
		friend std::basic_ostream<char, Traits> & operator << (std::basic_ostream<unicode_code_unit_utf8, Traits> & os, unicode_code_point x)
		{
			unicode_code_unit_utf8 chars[4];
			return os.write(chars, x.get(chars));
		}

		template <typename Traits>
		friend std::basic_ostream<char16_t, Traits> & operator << (std::basic_ostream<unicode_code_unit_utf16, Traits> & os, unicode_code_point x)
		{
			unicode_code_unit_utf16 chars[2];
			return os.write(chars, x.get(chars));
		}

		template <typename Traits>
		friend std::basic_ostream<char32_t, Traits> & operator << (std::basic_ostream<unicode_code_unit_utf32, Traits> & os, unicode_code_point x)
		{
			unicode_code_unit_utf32 chars[1];
			return os.write(chars, x.get(chars));
		}

#if defined(_MSC_VER) && defined(_UNICODE)
		template <typename Traits>
		friend std::basic_ostream<char16_t, Traits> & operator << (std::basic_ostream<unicode_code_unit_utfw, Traits> & os, unicode_code_point x)
		{
			unicode_code_unit_utfw chars[2];
			return os.write(chars, x.get(chars));
		}
#endif
	};

	struct encoding_utf8 {};
	struct encoding_utf16 {};
	struct encoding_utf32 {};
#if defined(_MSC_VER) && defined(_UNICODE)
	struct encoding_utfw {};
#endif

	template <>
	struct encoding_traits<encoding_utf8>
		: encoding_traits<unicode_code_unit_utf8>
	{
		using encoding_type = encoding_utf8;

		using buffer_type = std::array<unicode_code_unit_utf8, 4>;

		static constexpr ext::usize get(unicode_code_unit_utf8 code, unicode_code_unit_utf8 * buffer) { return buffer[0] = code; return 1; }
		static constexpr ext::usize get(unicode_code_point code, unicode_code_unit_utf8 * buffer) { return code.get(buffer); }
	};

	template <>
	struct encoding_traits<encoding_utf16>
		: encoding_traits<unicode_code_unit_utf16>
	{
		using encoding_type = encoding_utf16;

		using buffer_type = std::array<unicode_code_unit_utf16, 2>;

		static constexpr ext::usize get(unicode_code_unit_utf16 code, unicode_code_unit_utf16 * buffer) { return buffer[0] = code; return 1; }
		static constexpr ext::usize get(unicode_code_point code, unicode_code_unit_utf16 * buffer) { return code.get(buffer); }
	};

	template <>
	struct encoding_traits<encoding_utf32>
		: encoding_traits<unicode_code_unit_utf32>
	{
		using encoding_type = encoding_utf32;

		using buffer_type = std::array<unicode_code_unit_utf32, 1>;

		static constexpr ext::usize get(unicode_code_unit_utf32 code, unicode_code_unit_utf32 * buffer) { return buffer[0] = code; return 1; }
		static constexpr ext::usize get(unicode_code_point code, unicode_code_unit_utf32 * buffer) { return code.get(buffer); }
	};

#if defined(_MSC_VER) && defined(_UNICODE)
	template <>
	struct encoding_traits<encoding_utfw>
		: encoding_traits<unicode_code_unit_utfw>
	{
		using encoding_type = encoding_utfw;

		using buffer_type = std::array<unicode_code_unit_utfw, 2>;

		static constexpr ext::usize get(unicode_code_unit_utfw code, unicode_code_unit_utfw * buffer) { return buffer[0] = code; return 1; }
		static constexpr ext::usize get(unicode_code_point code, unicode_code_unit_utfw * buffer) { return code.get(buffer); }
	};
#endif

	using encoding_traits_utf8 = encoding_traits<encoding_utf8>;
	using encoding_traits_utf16 = encoding_traits<encoding_utf16>;
	using encoding_traits_utf32 = encoding_traits<encoding_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using encoding_traits_utfw = encoding_traits<encoding_utfw>;
#endif

	namespace detail
	{
		template <typename Encoding>
		struct boundary_unit_utf
			: boundary_unit<typename encoding_traits<Encoding>::value_type>
		{
			using encoding_type = Encoding;
		};

		template <typename Encoding>
		struct boundary_point_utf
		{
			using encoding_type = Encoding;

			using value_type = typename encoding_traits<Encoding>::value_type;
			using pointer = typename encoding_traits<Encoding>::pointer;
			using const_pointer = typename encoding_traits<Encoding>::const_pointer;
			using reference = unicode_code_point;
			using const_reference = unicode_code_point;
			using size_type = typename encoding_traits<Encoding>::size_type;
			using difference_type = typename encoding_traits<Encoding>::difference_type;

			static constexpr reference dereference(pointer p) { return reference(p); }
			static constexpr const_reference dereference(const_pointer p) { return const_reference(p); }

			static constexpr ext::ssize next(const_pointer p) { return unicode_code_point::extract_size(p); }
			static constexpr ext::ssize next(const_pointer p, ext::ssize count) { return next_impl(p, count, p); }

			static constexpr ext::ssize previous(const_pointer p) { return previous_impl(p); }
			static constexpr ext::ssize previous(const_pointer p, ext::ssize count) { return previous_impl(p, count, p); }

			static constexpr ext::usize length(const_pointer begin, const_pointer end) { return length_impl(begin, end, 0); }

		private:

			static constexpr ext::ssize next_impl(const_pointer p, ext::ssize count, const_pointer from)
			{
				return count <= 0 ? p - from : next_impl(p + next(p), count - 1, from);
			}

			static constexpr ext::ssize previous_impl(typename encoding_traits_utf8::const_pointer p)
			{
				// 0x80 = 1000 0000
				// 0xc0 = 1100 0000
				--p;
				if ((*p & 0xc0) != 0x80)
					return 1;

				--p;
				if ((*p & 0xc0) != 0x80)
					return 2;

				--p;
				if ((*p & 0xc0) != 0x80)
					return 3;

				return 4;
			}

			static constexpr ext::ssize previous_impl(typename encoding_traits_utf16::const_pointer p)
			{
				// 0xdc00 = 1101 1100 0000 0000
				// 0xfc00 = 1111 1100 0000 0000
				return (*p & 0xfc00) == 0xdc00 ? 2 : 1;
			}

			static constexpr ext::ssize previous_impl(typename encoding_traits_utf32::const_pointer) { return 1; }

#if defined(_MSC_VER) && defined(_UNICODE)
			static constexpr ext::ssize previous_impl(typename encoding_traits_utfw::const_pointer p)
			{
				// 0xdc00 = 1101 1100 0000 0000
				// 0xfc00 = 1111 1100 0000 0000
				return (*p & 0xfc00) == 0xdc00 ? 2 : 1;
			}
#endif

			static constexpr ext::ssize previous_impl(const_pointer p, ext::ssize count, const_pointer from)
			{
				return count <= 0 ? from - p : previous_impl(p - previous(p), count - 1, from);
			}

			static constexpr ext::usize length_impl(const_pointer begin, const_pointer end, ext::usize count)
			{
				return begin < end ? length_impl(begin + next(begin), end, count + 1) : count;
			}
		};
	}

	template <>
	struct boundary_unit<encoding_utf8>
		: detail::boundary_unit_utf<encoding_utf8>
	{};

	template <>
	struct boundary_unit<encoding_utf16>
		: detail::boundary_unit_utf<encoding_utf16>
	{};

	template <>
	struct boundary_unit<encoding_utf32>
		: detail::boundary_unit_utf<encoding_utf32>
	{};

#if defined(_MSC_VER) && defined(_UNICODE)
	template <>
	struct boundary_unit<encoding_utfw>
		: detail::boundary_unit_utf<encoding_utfw>
	{};
#endif

	template <>
	struct boundary_point<encoding_utf8>
		: detail::boundary_point_utf<encoding_utf8>
	{};

	template <>
	struct boundary_point<encoding_utf16>
		: detail::boundary_point_utf<encoding_utf16>
	{};

	template <>
	struct boundary_point<encoding_utf32>
		: detail::boundary_point_utf<encoding_utf32>
	{};

#if defined(_MSC_VER) && defined(_UNICODE)
	template <>
	struct boundary_point<encoding_utfw>
		: detail::boundary_point_utf<encoding_utfw>
	{};
#endif

	using boundary_unit_utf8 = boundary_unit<encoding_utf8>;
	using boundary_unit_utf16 = boundary_unit<encoding_utf16>;
	using boundary_unit_utf32 = boundary_unit<encoding_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using boundary_unit_utfw = boundary_unit<encoding_utfw>;
#endif

	using boundary_point_utf8 = boundary_point<encoding_utf8>;
	using boundary_point_utf16 = boundary_point<encoding_utf16>;
	using boundary_point_utf32 = boundary_point<encoding_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using boundary_point_utfw = boundary_point<encoding_utfw>;
#endif
}
