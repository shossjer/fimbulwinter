#pragma once

#include "utility/crypto/crc.hpp"

#include "ful/view.hpp"

namespace engine
{
#if MODE_DEBUG
	extern ful::unit_utf8 * debug_hashtable_copy(std::uint32_t value, ful::unit_utf8 * begin, ful::unit_utf8 * end);
#endif

	class Hash
	{
		using this_type = Hash;

	public:

		using value_type = std::uint32_t;

	protected:

		value_type value_;

	public:

		Hash() = default;

		explicit constexpr Hash(value_type value) : value_(value) {}

		explicit constexpr Hash(const char * str)
			: value_(utility::crypto::crc32(str))
		{}

		explicit constexpr Hash(const char * str, std::size_t n)
			: value_(utility::crypto::crc32(str, n))
		{}

		explicit constexpr Hash(ful::view_utf8 str)
			: value_(utility::crypto::crc32(str.data(), str.size()))
		{}

	public:

		constexpr operator value_type() const { return value_; }

	private:

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

		template <typename Stream>
		friend auto operator << (Stream && stream, this_type x)
			-> decltype(stream << x.value_)
		{
#if MODE_DEBUG
			ful::unit_utf8 chars[100]; // todo
			return stream << x.value_ << ful::view_utf8(chars + 0, debug_hashtable_copy(x.value_, chars + 0, chars + 100));
#else
			return stream << x.value_;
#endif
		}

	};
}
