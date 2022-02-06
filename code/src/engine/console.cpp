#include "engine/Asset.hpp"
#include "engine/console.hpp"
#include "engine/debug.hpp"

#include "fio/from_chars.hpp"

#include "ful/string_search.hpp"

namespace
{
	utility::heap_vector<engine::Asset, ext::heap_unique_ptr<engine::detail::CallbackBase>> observers;
}

namespace engine
{
	namespace detail
	{
		utility::heap_vector<Argument> parse_params(ful::view_utf8 line)
		{
			utility::heap_vector<Argument> params;

			auto begin = line.data();
			auto end = line.data() + line.size();

			while (begin != end)
			{
				if (*begin == '"')
				{
					const auto to = ful::find(begin + 1, end, ful::char8{'"'});
					if (to == end)
						break;

					if (!params.try_emplace_back(utility::in_place_type<ful::view_utf8>, begin + 1, to - begin - 1))
						break;

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
					if (!params.try_emplace_back(utility::in_place_type<bool>, true))
						break;
				}
				else if (word == "false")
				{
					if (!params.try_emplace_back(utility::in_place_type<bool>, false))
						break;
				}
				else
				{
					float tmpf; // todo double
					const char * retf = fio::from_chars(word.begin(), word.end(), tmpf);

					ext::ssize tmpi;
					const char * reti = fio::from_chars(word.begin(), word.end(), tmpi);

					if (reti == word.end())
					{
						if (!params.try_emplace_back(utility::in_place_type<ext::ssize>, tmpi))
							break;
					}
					else if (retf == word.end())
					{
						if (!params.try_emplace_back(utility::in_place_type<double>, static_cast<double>(tmpf)))
							break;
					}
					else
					{
						// todo report error
					}
				}

				begin = to;
			}

			return params;
		}

		void observe_impl(ful::view_utf8 keyword, ext::heap_unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset(keyword);
			if (!debug_assert(std::find_if(observers.begin(), observers.end(), [key](const auto & p){ return p.first == key; }) == observers.end(), "\"", keyword, "\" is being observed twice"))
				return;

			observers.try_emplace_back(key, std::move(callback));
		}

		void read_input(ful::view_utf8 line)
		{
			if (empty(line))
				return;

			const auto command_end = ful::find(line, ful::char8{' '});

			const auto command_name = ful::view_utf8(line.begin(), command_end);
			const engine::Asset command_key(command_name);

			const utility::heap_vector<Argument> params = parse_params(ful::view_utf8(command_end + 1, line.end()));

			for (auto && observer : observers)
			{
				if (observer.first == command_key)
				{
					observer.second->call(params);
					return;
				}
			}
			debug_printline("no matching observer found to \"", command_name, "\"");
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
