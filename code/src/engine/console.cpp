
#include "console.hpp"

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/Asset.hpp>
#include <engine/debug.hpp>

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
#else
	// TODO: linux stuff
#endif

}

namespace engine
{
	namespace console
	{
		void observe(
			const std::string & keyword,
			const Callback & callback)
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
			char buffer[256];

			while (active)
			{
				std::string line;

#ifdef _WIN32
				DWORD result;
				auto success = ReadFile(handle, buffer, 256, &result, NULL);

				if (!success || result == 0)
					break;

				line = std::string{ buffer };
#else
				// TODO: linux stuff
				break;
#endif
				std::stringstream stream{ line };
				std::string keyword;
				std::getline(stream, keyword, ' ');

				engine::Asset key{ keyword };

				auto observer = observer_map.find(engine::Asset{ keyword });

				if (observer != observer_map.end())
				{
					for (auto & callback : (*observer).second)
					{
						std::string data;
						std::getline(stream, data);
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

			if (handle == INVALID_HANDLE_VALUE)
			{
				debug_unreachable();
			}
#else
			// TODO: create linux stuff
#endif
			thread = core::async::Thread{ read_input };
		}

		void destroy()
		{
			active = false;
#ifdef _WIN32
			CloseHandle(handle);
#else
			// TODO: close linux stuff
#endif
			thread.join();
		}
	}
}
