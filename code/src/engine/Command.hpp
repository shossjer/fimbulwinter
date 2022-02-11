#pragma once

#include "core/serialization.hpp"

#include "engine/Hash.hpp"

namespace engine
{
	class Command : public Hash
	{
		using this_type = Command;

	public:

		Command() = default;

		explicit constexpr Command(const char * str)
			: Hash(str)
		{}

	public:

		static constexpr auto serialization()
		{
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("id"), &Command::value_)
				);
		}

	};
}
