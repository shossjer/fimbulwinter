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

		explicit Asset(utility::string_units_utf8 str)
			: Hash(str.data(), str.size())
		{
#if MODE_DEBUG
			add_string_to_hash_table(str.data(), str.size());
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
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("id"), &Asset::value_)
				);
		}

	};

	inline bool serialize(Asset & x, utility::string_units_utf8 object)
	{
		x = Asset(object);
		return true;
	}
}
