#pragma once

#include "config.h"

#include "utility/ext/stddef.hpp"

#if MODE_DEBUG
# include "ful/view.hpp"
#endif

namespace engine
{
#if MODE_DEBUG
	extern ful::unit_utf8 * debug_tokentable_copy(ext::usize value, ful::unit_utf8 * beg, ful::unit_utf8 * end);
#endif

	class Token
	{
		using this_type = Token;

	public:

		using value_type = ext::usize;

	private:

		value_type value_;

	public:

		Token() = default;

		explicit constexpr Token(value_type value)
			: value_(value)
		{}

	public:

		constexpr value_type value() const { return value_; }

	private:

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

		template <typename Stream>
		friend auto operator << (Stream && stream, this_type x)
			-> decltype(stream << value_type{})
		{
#if MODE_DEBUG
			ful::unit_utf8 chars[100]; // todo
			return stream << x.value_ << ful::view_utf8(chars + 0, debug_tokentable_copy(x.value_, chars + 0, chars + 100));
#else
			return stream << x.value_;
#endif
		}

	};
}
