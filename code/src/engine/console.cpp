#include "console.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"

#include "fio/stdio.hpp"

#include "ful/string_search.hpp"

#include <algorithm>
#include <vector>

namespace
{
	std::vector<std::pair<engine::Asset, std::unique_ptr<engine::detail::CallbackBase>>> observers;
}

namespace engine
{
	namespace detail
	{
		std::vector<Argument> parse_params(ful::view_utf8 line)
		{
			std::vector<Argument> params;

			auto begin = line.data();
			auto end = line.data() + line.size();

			while (begin != end)
			{
				if (*begin == '"')
				{
					const auto to = ful::find(begin + 1, end, ful::char8{'"'});
					if (to == end)
						throw std::runtime_error("missing ending quote (\") on string argument");

					params.emplace_back(utility::in_place_type<ful::view_utf8>, begin + 1, to - begin - 1);
					begin = to + 1; // '"'
					continue;
				}

				const auto to = ful::find(begin, end, ful::char8{' '});
				if (to == begin)
				{
					++begin; // ' '
					continue;
				}

				const auto word = ful::view_utf8(begin, to);
				if (word == "true")
				{
					params.emplace_back(utility::in_place_type<bool>, true);
				}
				else if (word == "false")
				{
					params.emplace_back(utility::in_place_type<bool>, false);
				}
				else if (ful::find(begin, to, ful::char8{'.'}) != to)
				{
					// todo error handling
					params.emplace_back(utility::in_place_type<double>, std::strtod(begin, nullptr));
				}
				else
				{
					// todo error handling
					// todo fio::from_chars
					params.emplace_back(utility::in_place_type<int64_t>, std::strtoll(begin, nullptr, 0));
				}
				begin = to;
			}

			return params;
		}

		void observe_impl(ful::view_utf8 keyword, std::unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset(keyword);
			if (!debug_assert(std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; }) == observers.end(), "\"", keyword, "\" is being observed twice"))
				return;

			observers.emplace_back(key, std::move(callback));
		}

		void read_input(ful::view_utf8 line)
		{
			if (empty(line))
				return;

			const auto command_end = ful::find(line, ful::char8{' '});

			const auto command_name = ful::view_utf8(line.begin(), command_end);
			const engine::Asset command_key(command_name);

			const std::vector<Argument> params = parse_params(ful::view_utf8(command_end + 1, line.end()));

			for (auto & observer : observers)
			{
				if (observer.first == command_key)
				{
					observer.second->call(params);
					return;
				}
			}
			fio::stdostream{} << "no matching observer found to \"" << command_name << "\"\n";
		}
	}

	void abandon(ful::view_utf8 keyword)
	{
		const auto key = engine::Asset(keyword);
		const auto it = std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; });
		debug_assert(it != observers.end(), "\"", keyword, "\" is not being observed");

		observers.erase(it);
	}
}
