#include "console.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
	std::vector<std::pair<engine::Asset, std::unique_ptr<engine::detail::CallbackBase>>> observers;
}

namespace engine
{
	namespace detail
	{
		std::vector<Argument> parse_params(utility::string_view_utf8 line)
		{
			std::vector<Argument> params;

			while (!line.empty())
			{
				if (line.front() == '"')
				{
					const auto to = utility::unit_difference(line.find('"', utility::unit_difference(1)));
					if (to == line.size())
						throw std::runtime_error("missing ending quote (\") on string argument");

					params.emplace_back(utility::in_place_type<utility::string_view_utf8>, line, utility::unit_difference(1), to - 1);
					line = utility::string_view_utf8(line, to + 1);
					continue;
				}

				auto to = utility::unit_difference(line.find(' '));
				if (to == 0)
				{
					line = utility::string_view_utf8(line, utility::unit_difference(1));
					continue;
				}

				auto word = utility::string_view_utf8(line, utility::unit_difference(0), to);
				if (word.compare("true") == 0)
				{
					params.emplace_back(utility::in_place_type<bool>, true);
				}
				else if (word.compare("false") == 0)
				{
					params.emplace_back(utility::in_place_type<bool>, false);
				}
				else if (utility::unit_difference(line.find('.')) < to)
				{
					// todo error handling
					params.emplace_back(utility::in_place_type<double>, std::strtod(line.data(), nullptr));
				}
				else
				{
					// todo error handling
					params.emplace_back(utility::in_place_type<int64_t>, std::strtoll(line.data(), nullptr, 0));
				}
				line = utility::string_view_utf8(line, to);
			}

			return params;
		}

		void observe_impl(utility::string_view_utf8 keyword, std::unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset(keyword);
			if (!debug_assert(std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; }) == observers.end(), "\"", keyword, "\" is being observed twice"))
				return;

			observers.emplace_back(key, std::move(callback));
		}

		void read_input(utility::string_view_utf8 line)
		{
			if (line.empty())
				return;

			const auto command_end = utility::unit_difference(line.find(' '));

			const auto command_name = utility::string_view_utf8(line, utility::unit_difference(0), command_end);
			const engine::Asset command_key(command_name);

			const std::vector<Argument> params = parse_params(utility::string_view_utf8(line, command_end + 1));

			for (auto & observer : observers)
			{
				if (observer.first == command_key)
				{
					observer.second->call(params);
					return;
				}
			}
			std::cout << "no matching observer found to \"" << command_name << "\"\n";
		}
	}

	void abandon(utility::string_view_utf8 keyword)
	{
		const auto key = engine::Asset(keyword);
		const auto it = std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; });
		debug_assert(it != observers.end(), "\"", keyword, "\" is not being observed");

		observers.erase(it);
	}
}
