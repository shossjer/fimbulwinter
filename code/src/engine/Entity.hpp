#pragma once

#include "engine/Token.hpp"

namespace engine
{
	class Entity
	{
		using this_type = Entity;

	public:

		using value_type = ext::usize;

	private:

		value_type value_;

	public:

		Entity() = default;

		explicit constexpr Entity(value_type value)
			: value_{value}
		{}

	public:

		constexpr value_type value() const { return value_; }

		constexpr operator Token () const { return Token(value_); }

	private:

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

		template <typename Stream>
		friend auto operator << (Stream & stream, this_type x)
			-> decltype(stream << value_type{})
		{
#if defined(_DEBUG) || !defined(NDEBUG)
			ful::unit_utf8 chars[100]; // todo
			return stream << x.value_ << ful::view_utf8(chars + 0, debug_tokentable_copy(x.value_, chars + 0, chars + 100));
#else
			return stream << x.value_;
#endif
		}

	};
}
