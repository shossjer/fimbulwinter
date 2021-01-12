#pragma once

#include "config.h"

#include "utility/concepts.hpp"
#include "utility/type_info.hpp"

#if MODE_DEBUG
# include <ostream>
#endif

namespace engine
{
	class Token
	{
		using this_type = Token;

	public:

		using value_type = std::uint32_t;

	public:

#if MODE_DEBUG
		// note debug only
		static std::ostream & (* ostream_debug)(std::ostream & stream, this_type token);
#endif

	private:

		value_type value_;

#if MODE_DEBUG
		utility::type_id_t type_;
#endif

	public:

		Token() = default;

#if MODE_DEBUG
		explicit constexpr Token(value_type value, utility::type_id_t type)
			: value_(value)
			, type_(type)
		{}
#else
		explicit constexpr Token(value_type value, utility::type_id_t /*type*/)
			: value_(value)
		{}
#endif

		template <typename P,
		          typename T = mpl::remove_cvref_t<P>,
		          REQUIRES((!mpl::is_same<this_type, T>::value)),
		          REQUIRES((std::is_constructible<value_type, P>::value))>
		constexpr Token(P && p)
			: value_(std::forward<P>(p))
#if MODE_DEBUG
			, type_(utility::type_id<T>())
#endif
		{}

	public:

		constexpr value_type value() const { return value_; }

#if MODE_DEBUG
		// note debug only
		constexpr utility::type_id_t type() const { return type_; }
#endif

		// todo remove
		constexpr operator value_type () const { return value_; }

	public:

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

#if MODE_DEBUG
		// note debug only
		friend std::ostream & operator << (std::ostream & stream, this_type token)
		{
			return ostream_debug(stream, token);
		}
#endif

	};
}
