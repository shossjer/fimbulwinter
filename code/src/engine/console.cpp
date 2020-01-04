
#include "console.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"

#include "utility/string.hpp"
#include "utility/string_view.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
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
		std::vector<Argument> parse_params(const std::string & line, std::ptrdiff_t from)
		{
			std::vector<Argument> params;

			while (std::size_t(from) < line.length())
			{
				if (line[from] == '"')
				{
					const std::ptrdiff_t to = line.find('"', from + 1);
					if (std::size_t(to) == std::string::npos)
						throw std::runtime_error("missing ending quote (\") on string argument");

					params.emplace_back(utility::in_place_type<utility::string_view>, line.data() + from + 1, to - from - 1);
					from = to + 1;
					continue;
				}

				std::ptrdiff_t to = line.find(' ', from);
				if (from == to)
				{
					from++;
					continue;
				}
				if (std::size_t(to) == std::string::npos)
				{
					to = line.length();
				}

				utility::string_view word(line.data() + from, to - from);
				if (word.compare("true") == 0)
				{
					params.emplace_back(utility::in_place_type<bool>, true);
				}
				else if (word.compare("false") == 0)
				{
					params.emplace_back(utility::in_place_type<bool>, false);
				}
				else if (line.find('.', from) < std::size_t(to))
				{
					try
					{
						params.emplace_back(utility::in_place_type<double>, std::stod(line.substr(from, to - from)));
					}
					catch (const std::invalid_argument &)
					{
						throw std::runtime_error(utility::to_string("at ", from, ": argument not a floating type"));
					}
				}
				else
				{
					try
					{
						params.emplace_back(utility::in_place_type<int64_t>, std::stoll(line.substr(from, to - from)));
					}
					catch (const std::invalid_argument &)
					{
						throw std::runtime_error(utility::to_string("at ", from, ": argument not a integral type"));
					}
				}
				from = to + 1;
			}

			return params;
		}

		void observe_impl(utility::string_view keyword, std::unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset(keyword);
			const auto it = std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; });
			debug_assert(it == observers.end(), "\"", keyword, "\" is being observed twice");

			observers.emplace_back(key, std::move(callback));
		}

		void read_input(std::string line)
		{
			if (line.empty())
				return;

			const std::ptrdiff_t command_begin = 0;
			std::ptrdiff_t command_end = line.find(' ', command_begin);
			if (std::size_t(command_end) == std::string::npos)
			{
				command_end = line.length();
			}

			const utility::string_view command_name(line.data() + command_begin, command_end - command_begin);
			const engine::Asset command_key(command_name);

			const std::vector<Argument> params = parse_params(line, command_end + 1);

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

	void abandon(utility::string_view keyword)
	{
		const auto key = engine::Asset(keyword);
		const auto it = std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; });
		debug_assert(it != observers.end(), "\"", keyword, "\" is not being observed");

		observers.erase(it);
	}
}
