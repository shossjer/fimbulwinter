
#include "reader.hpp"

#include "core/debug.hpp"
#include "core/serialize.hpp"
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

template <typename S>
void serialize(S & s, utility::any & data, engine::Command command)
{
	using core::serialize;

	switch (command)
	{
	case engine::command::RENDER_DESELECT:
	case engine::command::RENDER_HIGHLIGHT:
	case engine::command::RENDER_SELECT:
		serialize(s, data.emplace<engine::Entity>());
		break;
	default:
		;
	}
}

namespace
{
	template <typename Structurer>
	std::tuple<int, engine::Entity, engine::Command, utility::any>
	parse_next_command(Structurer & s)
	{
		using core::tuple_begin;
		using core::tuple_end;
		using core::tuple_space;
		using core::newline;
		using core::serialize;
		using ::serialize;

		std::tuple<int, engine::Entity, engine::Command, utility::any> command_args;
		std::get<0>(command_args) = INT32_MAX;
		s(tuple_begin);
		serialize(s, std::get<0>(command_args));
		debug_printline("std::get<0>(command_args) = ", std::get<0>(command_args));
		s(tuple_space);
		serialize(s, std::get<1>(command_args));
		debug_printline("std::get<1>(command_args) = ", std::get<1>(command_args));
		s(tuple_space);
		serialize(s, std::get<2>(command_args));
		debug_printline("std::get<2>(command_args) = ", std::get<2>(command_args));
		s(tuple_space);
		serialize(s, std::get<3>(command_args), std::get<2>(command_args));
		debug_printline("std::get<3>(command_args) = ", std::get<3>(command_args));
		s(tuple_end);
		s(newline);
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
