
#include "console.hpp"

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/Asset.hpp>
#include <engine/debug.hpp>

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace
{
	using namespace engine::console;

	std::vector < std::pair<engine::Asset, std::unique_ptr<CallbackBase>> > observers;

	volatile bool active;
	core::async::Thread thread;

#ifdef _WIN32
	HANDLE handle = nullptr;
#endif
}

namespace engine
{
	namespace console
	{
		void observe_impl(const std::string & keyword, std::unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset{ keyword };

			observers.emplace_back(key, std::move(callback));
		}

		auto parse_params(const std::string & line)
		{
			std::vector<Param> params;

			// TODO: actually parse the string...
			params.emplace_back(utility::in_place_type<std::string>, line);

			return params;
		}

		void read_input()
		{
			while (active)
			{
				std::string line;
				std::getline(std::cin, line);

				if (line.empty()) continue;

				std::stringstream stream{ line };
				std::string data;

				std::getline(stream, data, ' ');
				const engine::Asset key{ data };

				std::getline(stream, data);
				std::vector<Param> params = parse_params(data);

				for (auto & observer : observers)
				{
					if (observer.first == key)
						observer.second->call(params);
				}
			}
		}

		void create()
		{
			active = true;
#ifdef _WIN32
			handle = GetStdHandle(STD_INPUT_HANDLE);
#else
			// TODO: create linux stuff
#endif
			thread = core::async::Thread{ read_input };
		}

		void destroy()
		{
			active = false;
#ifdef _WIN32
			CancelIoEx(handle, NULL);
#else
			// TODO: close linux stuff
#endif
			thread.join();
		}
	}
}
