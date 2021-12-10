#pragma once

#include "utility/crypto/crc.hpp"

#include "ful/view.hpp"

#include <cstdint>

namespace engine
{
	class Hash
	{
		using this_type = Hash;

	public:

		using value_type = std::uint32_t;

	protected: // todo needed for serialization

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

		constexpr value_type value() const { return value_; }

		constexpr operator value_type () const { return value_; }

	private:

#if MODE_DEBUG
		static ful::view_utf8 get_string_from_hash_table(Hash hash);
#endif

		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }

		template <typename Stream>
		friend decltype(auto) operator << (Stream & stream, this_type x)
		{
#if MODE_DEBUG
			return stream << x.value_ << get_string_from_hash_table(x);
#else
			return stream << x.value_;
#endif
		}

	};
}
