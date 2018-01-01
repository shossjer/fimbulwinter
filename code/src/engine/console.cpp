
#include "console.hpp"

#include <engine/Asset.hpp>
#include <engine/debug.hpp>

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <cctype>
#include <cstdio>

namespace
{
	using namespace engine::console;

	std::vector < std::pair<engine::Asset, std::unique_ptr<CallbackBase>> > observers;
}

namespace engine
{
	namespace console
	{
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

		void observe_impl(const std::string & keyword, std::unique_ptr<CallbackBase> && callback)
		{
			const auto key = engine::Asset{ keyword };

			observers.emplace_back(key, std::move(callback));
		}

		void read_input(std::string line)
		{
			if (line.empty())
				return;

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
}
