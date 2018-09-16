
#ifndef ENGINE_COMMAND_HPP
#define ENGINE_COMMAND_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

namespace engine
{
	class Command
	{
	private:
		using this_type = Command;

	private:
		engine::Asset asset;

	public:
		Command() = default;
		template <std::size_t N>
		constexpr Command(const char (& str)[N])
			: asset(str)
		{}
		constexpr Command(const char *const str, const std::size_t n)
			: asset(str, n)
		{}

	public:
		constexpr operator engine::Asset::value_type () const
		{
			return asset;
		}

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("asset"), &Command::asset)
				);
		}

		friend std::ostream & operator << (std::ostream & stream, const this_type & command)
		{
			return stream << command.asset;
		}
	};
}

#endif /* ENGINE_COMMAND_HPP */
