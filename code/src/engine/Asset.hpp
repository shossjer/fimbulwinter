#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "engine/Hash.hpp"

namespace engine
{
	class Asset : public Hash
	{
		using this_type = Asset;

	public:

		using value_type = Hash::value_type;

	public:

		Asset() = default;

		explicit Asset(value_type value) : Hash(value) {}

		explicit Asset(const char * str)
			: Hash(str)
		{
#if MODE_DEBUG
			add_string_to_hash_table(str);
#endif
		}

		explicit Asset(const char * str, std::size_t n)
			: Hash(str, n)
		{
#if MODE_DEBUG
			add_string_to_hash_table(str, n);
#endif
		}

		template <typename R, typename = decltype(data(std::declval<R>()), size(std::declval<R>()))>
		explicit Asset(const R & str)
			: Hash(data(str), size(str))
		{
#if MODE_DEBUG
			add_string_to_hash_table(data(str), size(str));
#endif
		}

	private:

#if MODE_DEBUG
		void add_string_to_hash_table(const char * str);

		void add_string_to_hash_table(const char * str, std::size_t n);
#endif

	public:

		static constexpr auto serialization()
		{
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("id"), &Asset::value_)
				);
		}

	};

	inline bool serialize(Asset & x, ful::view_utf8 object)
	{
		x = Asset(object);
		return true;
	}

	inline bool serialize(Asset & x, ful::cstr_utf8 object)
	{
		return serialize(x, static_cast<ful::view_utf8>(object));
	}
}
