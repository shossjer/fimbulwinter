
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

	std::unordered_map<engine::Asset, std::vector<Callback>> observer_map;

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
		void observe(const std::string & keyword, const Callback & callback)
		{
			const auto key = engine::Asset{ keyword };

			auto itr = observer_map.find(key);

			if (itr == observer_map.end())
			{
				auto r = observer_map.insert({ key, {} });

				debug_assert(r.second);

				r.first->second.push_back(callback);
			}
			else
			{
				(*itr).second.emplace_back(callback);
			}
		}

		void read_input()
		{
			while (active)
			{
				std::string line;
				std::getline(std::cin, line);

				if (line.empty()) continue;

				std::stringstream stream{ line };
				std::string keyword;
				std::getline(stream, keyword, ' ');

				const engine::Asset key{ keyword };

				auto observers = observer_map.find(engine::Asset{ keyword });

				if (observers != observer_map.end())
				{
					std::string data;
					std::getline(stream, data);

					for (auto & callback : (*observers).second)
					{
						callback(data);
					}
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
