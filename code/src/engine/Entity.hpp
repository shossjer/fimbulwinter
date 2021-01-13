#pragma once

#include "config.h"

#include <cstdint>

#if MODE_DEBUG
# include <ostream>
#endif

namespace engine
{
	class Entity
	{
		using this_type = Entity;

	public:

		using value_type = std::uint32_t;

	private:

		value_type value_;

	public:

		Entity() = default;

		explicit Entity(value_type value) : value_{value} {}

	public:

		value_type value() const { return value_; }

		operator value_type () const { return value_; }

	private:

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

#if MODE_DEBUG
		// note debug only
		friend std::ostream & operator << (std::ostream & stream, this_type x)
		{
			return stream << x.value_;
		}
#endif

	};
}
