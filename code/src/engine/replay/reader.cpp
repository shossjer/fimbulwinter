
#include "reader.hpp"

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/StringStructurer.hpp"

#include "engine/commands.hpp"
#include "engine/Entity.hpp"

#include "utility/any.hpp"

#include <fstream>
#include <vector>

namespace gameplay
{
	namespace gamestate
	{
		void post_command(engine::Entity entity, engine::Command command, utility::any && data);
	}
}

namespace
{
	template <typename Structurer>
	void parse_data(Structurer & s, utility::any & data, engine::Command command)
	{
		switch (command)
		{
		case engine::command::RENDER_DESELECT:
		case engine::command::RENDER_HIGHLIGHT:
		case engine::command::RENDER_SELECT:
			s.read(data.emplace<engine::Entity>());
			break;
		default:
			break;
		}
	}

	template <typename Structurer>
	std::tuple<int, engine::Entity, engine::Command, utility::any>
	parse_next_command(Structurer & s)
	{
		std::tuple<int, engine::Entity, engine::Command, utility::any> command_args;
		std::get<0>(command_args) = INT32_MAX;
		s.read_as_tuple(std::get<0>(command_args), std::get<1>(command_args), std::get<2>(command_args), [&](Structurer & s_){ parse_data(s_, std::get<3>(command_args), std::get<2>(command_args)); });
		return command_args;
	}

	core::StringStructurer structurer;
	std::tuple<int, engine::Entity, engine::Command, utility::any> next_command;

	void load()
	{
		std::ifstream file("replay.rec", std::ofstream::binary);
		if (!file)
		{
			debug_printline("could not open \"replay.rec\" for replay");
			std::get<0>(next_command) = INT32_MAX;
			return;
		}

		debug_printline("replaying \"replay.rec\"");

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		std::vector<char> bytes;
		bytes.resize(file_size);

		file.read(bytes.data(), bytes.size());

		structurer.set(bytes.data(), bytes.size());

		next_command = parse_next_command(structurer);
	}
}

namespace engine
{
	namespace replay
	{
		void start()
		{
			load();
		}

		void update(int frame_count)
		{
			while (std::get<0>(next_command) <= frame_count)
			{
				gameplay::gamestate::post_command(std::get<1>(next_command), std::get<2>(next_command), std::move(std::get<3>(next_command)));

				next_command = parse_next_command(structurer);
			}
		}
	}
}
