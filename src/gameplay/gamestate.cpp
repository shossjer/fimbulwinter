
#include "gamestate.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <engine/graphics/viewer.hpp>
#include <gameplay/ui.hpp>

using gameplay::gamestate::Command;

namespace
{
	struct CameraActivator
	{
		engine::Entity camera;

		CameraActivator(engine::Entity camera) : camera(camera) {}

		void operator = (Command command)
		{
			switch (command)
			{
			case Command::CONTEXT_CHANGED:
				debug_printline(0xffffffff, "Switching to camera: ", camera);
				engine::graphics::viewer::set_active_3d(camera);
				break;
			default:
				debug_printline(0xffffffff, "CameraActivator: Unknown command: ", static_cast<int>(command));
			}
		}
	};

	struct FreeCamera
	{
		engine::Entity camera;

		bool move_left = false;
		bool move_right = false;
		bool move_down = false;
		bool move_up = false;
		bool turn_left = false;
		bool turn_right = false;
		bool turn_down = false;
		bool turn_up = false;
		bool roll_left = false;
		bool roll_right = false;

		FreeCamera(engine::Entity camera) : camera(camera) {}

		void operator = (Command command)
		{
			switch (command)
			{
			case Command::MOVE_LEFT_DOWN:
				move_left = true;
				break;
			case Command::MOVE_LEFT_UP:
				move_left = false;
				break;
			case Command::MOVE_RIGHT_DOWN:
				move_right = true;
				break;
			case Command::MOVE_RIGHT_UP:
				move_right = false;
				break;
			case Command::MOVE_DOWN_DOWN:
				move_down = true;
				break;
			case Command::MOVE_DOWN_UP:
				move_down = false;
				break;
			case Command::MOVE_UP_DOWN:
				move_up = true;
				break;
			case Command::MOVE_UP_UP:
				move_up = false;
				break;
			case Command::TURN_LEFT_DOWN:
				turn_left = true;
				break;
			case Command::TURN_LEFT_UP:
				turn_left = false;
				break;
			case Command::TURN_RIGHT_DOWN:
				turn_right = true;
				break;
			case Command::TURN_RIGHT_UP:
				turn_right = false;
				break;
			case Command::TURN_DOWN_DOWN:
				turn_down = true;
				break;
			case Command::TURN_DOWN_UP:
				turn_down = false;
				break;
			case Command::TURN_UP_DOWN:
				turn_up = true;
				break;
			case Command::TURN_UP_UP:
				turn_up = false;
				break;
			case Command::ROLL_LEFT_DOWN:
				roll_left = true;
				break;
			case Command::ROLL_LEFT_UP:
				roll_left = false;
				break;
			case Command::ROLL_RIGHT_DOWN:
				roll_right = true;
				break;
			case Command::ROLL_RIGHT_UP:
				roll_right = false;
				break;
			default:
				debug_printline(0xffffffff, "FreeCamera: Unknown command: ", static_cast<int>(command));
			}
		}

		void update()
		{
			if (move_left)
				engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::translate({-1.f, 0.f, 0.f}));
			if (move_right)
				engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::translate({1.f, 0.f, 0.f}));
			if (move_up)
				engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::translate({0.f, 0.f, -1.f}));
			if (move_down)
				engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::translate({0.f, 0.f, 1.f}));
		}
	};

	core::container::Collection
	<
		engine::Entity,
		5,
		std::array<CameraActivator, 2>,
		std::array<FreeCamera, 2>
	>
	components;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, Command>, 100> queue_commands;
}

namespace gameplay
{
namespace gamestate
{
	void create()
	{
		gameplay::ui::post_add_context("debug");
		gameplay::ui::post_add_context("game");
		gameplay::ui::post_activate_context("game");

		auto debug_camera = engine::Entity::create();
		auto game_camera = engine::Entity::create();

		components.emplace<FreeCamera>(debug_camera, debug_camera);
		components.emplace<FreeCamera>(game_camera, game_camera);

		engine::graphics::viewer::add(
				debug_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ 0.766f, 0.643f, 0.f, 0.f },
					core::maths::Vector3f{0.f, 4.f, 0.f}});
		engine::graphics::viewer::add(
				game_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ 0.966f, 0.259f, 0.f, 0.f },
					core::maths::Vector3f{0.f, 4.f, 5.f}});
		engine::graphics::viewer::set_active_3d(game_camera);

		gameplay::ui::post_add_flycontrol(debug_camera);
		gameplay::ui::post_add_pancontrol(game_camera);
		gameplay::ui::post_bind("debug", debug_camera, 0);
		gameplay::ui::post_bind("game", game_camera, 0);

		auto debug_switch = engine::Entity::create();
		auto game_switch = engine::Entity::create();

		components.emplace<CameraActivator>(debug_switch, debug_camera);
		components.emplace<CameraActivator>(game_switch, game_camera);

		gameplay::ui::post_add_contextswitch(debug_switch, engine::hid::Input::Button::KEY_F1, "debug");
		gameplay::ui::post_add_contextswitch(game_switch, engine::hid::Input::Button::KEY_F2, "game");
		gameplay::ui::post_bind("debug", game_switch, -10);
		gameplay::ui::post_bind("game", debug_switch, -10);
	}

	void destroy()
	{

	}

	void update()
	{
		// commands
		{
			std::pair<engine::Entity, Command> command;
			while (queue_commands.try_pop(command))
			{
				components.update(command.first, command.second);
			}
		}

		// update
		for (auto & camera : components.get<FreeCamera>())
		{
			camera.update();
		}
	}

	void post_command(engine::Entity entity, Command command)
	{
		const auto res = queue_commands.try_emplace(entity, command);
		debug_assert(res);
	}
}
}
