#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "engine/Hash.hpp"

namespace engine
{
#if MODE_DEBUG
	extern void debug_hashtable_add(std::uint32_t value, ful::view_utf8 string);
#endif

	class Asset : public Hash
	{
		using this_type = Asset;

	public:

		using value_type = Hash::value_type;

	public:

		Asset() = default;

		explicit constexpr Asset(value_type value)
			: Hash(value)
		{}

		explicit constexpr Asset(Hash hash)
			: Hash(hash)
		{}

		explicit Asset(const char * str, std::size_t n)
			: Hash(str, n)
		{
#if MODE_DEBUG
			debug_hashtable_add(static_cast<std::uint32_t>(*this), ful::view_utf8(str, n));
#endif
		}

		template <unsigned long long N>
		explicit constexpr Asset(const char (& str)[N])
			: Hash(str, N - 1) // subtract terminating null
		{
#if MODE_DEBUG
			debug_hashtable_add(static_cast<std::uint32_t>(*this), ful::view_utf8(str, N - 1));
#endif
		}

		template <typename R, typename = decltype(data(std::declval<R>()), size(std::declval<R>()))>
		explicit Asset(const R & str)
			: Hash(data(str), size(str))
		{
#if MODE_DEBUG
			debug_hashtable_add(static_cast<std::uint32_t>(*this), ful::view_utf8(data(str), size(str)));
#endif
		}

	public:

		static constexpr auto serialization()
		{
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("id"), &Asset::value_)
				);
		}

	private:

		template <typename Stream>
		friend Stream && operator >> (Stream && stream, this_type & value)
		{
			value = Asset(stream.buffer());

			// todo consume whole stream buffer

			return static_cast<Stream &&>(stream);
		}

	};
}
