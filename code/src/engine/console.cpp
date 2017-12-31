
#include "console.hpp"

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include "engine/application/window.hpp"
#include <engine/Asset.hpp>
#include <engine/debug.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <cctype>
#include <stdio.h>

#ifndef _WIN32
# include <poll.h>
# include <unistd.h>
#endif

namespace
{
	using namespace engine::console;

	std::vector < std::pair<engine::Asset, std::unique_ptr<CallbackBase>> > observers;

	core::async::Thread thread;

#ifdef _WIN32
	volatile bool active;
	HANDLE handle = nullptr;
#else
	int interrupt_pipes[2];
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

		bool is_number_candidate(const std::string & str)
		{
			return str.find_first_not_of("-.0123456789") == std::string::npos;
		}

		auto parse_params(const std::string & raw_line)
		{
			std::vector<Param> params;

			std::stringstream stream{ raw_line };

			while (!stream.eof())
			{
				std::string data;

				if (stream.peek() == '"')
				{
					stream.get(); // step over first char
					std::getline(stream, data, '"');

					if (data.empty()) continue;

					params.emplace_back(utility::in_place_type<std::string>, data);
				}
				else
				{
					std::getline(stream, data, ' ');

					if (data.empty()) continue;

					if (is_number_candidate(data))
					{
						try
						{
							if (data.find(".") == std::string::npos)
							{
								int value = std::stoi(data);
								params.emplace_back(utility::in_place_type<int>, value);
							}
							else
							{
								float value = std::stof(data);
								params.emplace_back(utility::in_place_type<float>, value);
							}
						}
						catch (const std::invalid_argument & e)
						{
							debug_printline("Could not parse: ", data, " as number: ", e.what());
							params.emplace_back(utility::in_place_type<std::string>, data);
						}
					}
					else if ("false" == data)
					{
						params.emplace_back(utility::in_place_type<bool>, false);
					}
					else if ("true" == data)
					{
						params.emplace_back(utility::in_place_type<bool>, true);
					}
					else
					{
						params.emplace_back(utility::in_place_type<std::string>, data);
					}
				}
			}

			return params;
		}

		void read_input()
		{
#ifdef _WIN32
			while (active)
			{
				std::string line;
				if (!std::getline(std::cin, line))
				{
					application::window::close();
					break;
				}

				if (line.empty())
					continue;

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
#else
			struct pollfd fds[2] = {
				{interrupt_pipes[0], 0, 0},
				{STDIN_FILENO, POLLIN, 0}
			};

			while (true)
			{
				const auto n = poll(fds, 2, -1);

				if (fds[0].revents & POLLHUP)
					break;

				if (fds[1].revents & POLLIN)
				{
					std::string line;
					if (!std::getline(std::cin, line))
					{
						application::window::close();
						break;
					}

					if (line.empty())
						continue;

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
			close(interrupt_pipes[0]);
#endif
			debug_printline("console thread stopping");
		}

		void create()
		{
#ifdef _WIN32
			active = true;
			handle = GetStdHandle(STD_INPUT_HANDLE);
#else
			pipe(interrupt_pipes);
#endif
			thread = core::async::Thread{ read_input };
		}

		void destroy()
		{
#ifdef _WIN32
			active = false;
			CancelIoEx(handle, NULL);
#else
			close(interrupt_pipes[1]);
#endif
			thread.join();
		}
	}
}
