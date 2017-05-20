
#include "gamestate.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>

#include <engine/graphics/viewer.hpp>

#include <gameplay/ui.hpp>

namespace
{
	void printClick(engine::Entity id);

	struct CameraActivator
	{
		engine::Entity camera;

		CameraActivator(engine::Entity camera) : camera(camera) {}

		void operator = (std::tuple<engine::Command, int> && args)
		{
			switch (std::get<0>(args))
			{
			case engine::Command::CONTEXT_CHANGED:
				debug_printline(0xffffffff, "Switching to camera: ", camera);
				engine::graphics::viewer::set_active_3d(camera);
				break;
			default:
				debug_printline(0xffffffff, "CameraActivator: Unknown command: ", static_cast<int>(std::get<0>(args)));
			}
		}
		void operator = (std::tuple<engine::Command, engine::Entity> && args)
		{
			debug_unreachable();
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

		void operator = (std::tuple<engine::Command, int> && args)
		{
			switch (std::get<0>(args))
			{
			case engine::Command::MOVE_LEFT_DOWN:
				move_left = true;
				break;
			case engine::Command::MOVE_LEFT_UP:
				move_left = false;
				break;
			case engine::Command::MOVE_RIGHT_DOWN:
				move_right = true;
				break;
			case engine::Command::MOVE_RIGHT_UP:
				move_right = false;
				break;
			case engine::Command::MOVE_DOWN_DOWN:
				move_down = true;
				break;
			case engine::Command::MOVE_DOWN_UP:
				move_down = false;
				break;
			case engine::Command::MOVE_UP_DOWN:
				move_up = true;
				break;
			case engine::Command::MOVE_UP_UP:
				move_up = false;
				break;
			case engine::Command::TURN_LEFT_DOWN:
				turn_left = true;
				break;
			case engine::Command::TURN_LEFT_UP:
				turn_left = false;
				break;
			case engine::Command::TURN_RIGHT_DOWN:
				turn_right = true;
				break;
			case engine::Command::TURN_RIGHT_UP:
				turn_right = false;
				break;
			case engine::Command::TURN_DOWN_DOWN:
				turn_down = true;
				break;
			case engine::Command::TURN_DOWN_UP:
				turn_down = false;
				break;
			case engine::Command::TURN_UP_DOWN:
				turn_up = true;
				break;
			case engine::Command::TURN_UP_UP:
				turn_up = false;
				break;
			case engine::Command::ROLL_LEFT_DOWN:
				roll_left = true;
				break;
			case engine::Command::ROLL_LEFT_UP:
				roll_left = false;
				break;
			case engine::Command::ROLL_RIGHT_DOWN:
				roll_right = true;
				break;
			case engine::Command::ROLL_RIGHT_UP:
				roll_right = false;
				break;
			default:
				debug_printline(0xffffffff, "FreeCamera: Unknown command: ", static_cast<int>(std::get<0>(args)));
			}
		}
		void operator = (std::tuple<engine::Command, engine::Entity> && args)
		{
			debug_unreachable();
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

	struct Selector
	{
		void operator = (std::tuple<engine::Command, int> && args)
		{
			debug_unreachable();
		}
		void operator = (std::tuple<engine::Command, engine::Entity> && args)
		{
			switch (std::get<0>(args))
			{
			case engine::Command::RENDER_SELECT:
				printClick(std::get<1>(args));
				break;
			default:
				debug_printline(0xffffffff, "Selector: Unknown command: ", static_cast<int>(std::get<0>(args)));
			}
		}
	};

	struct Worker
	{
		engine::Entity id;
		engine::Entity workstation;

		Worker(engine::Entity id) : id(id)
		{
		}
	};

	struct Workstation
	{
		engine::Entity id;
		gameplay::gamestate::WorkstationType type;
		engine::Entity worker;
		double progress;

		Workstation(engine::Entity id, gameplay::gamestate::WorkstationType type)
			: id(id)
			, type(type)
		{
		}

		bool isBusy()
		{
			return this->worker != engine::Entity::null();
		}
	};

	core::container::Collection
	<
		engine::Entity,
		141,
		std::array<CameraActivator, 2>,
		std::array<FreeCamera, 2>,
		std::array<Selector, 1>,
		std::array<Worker, 10>,
		std::array<Workstation, 20>
	>
	components;

	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Command>, 100> queue_commands0;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Command, engine::Entity>, 100> queue_commands1;

	core::container::CircleQueueSRMW<std::tuple<engine::Entity, gameplay::gamestate::WorkstationType>, 100> queue_workstations;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_workers;

	struct PrintType
	{
		void operator() (const Worker & w)
		{
			debug_printline(0xffffffff, "You have clicked worker!");
		}

		void operator() (const Workstation & w)
		{
			switch (w.type)
			{
			case gameplay::gamestate::WorkstationType::BENCH:
				debug_printline(0xffffffff, "You have clicked Bench.");
				break;
			case gameplay::gamestate::WorkstationType::OVEN:
				debug_printline(0xffffffff, "You have clicked Oven.");
				break;

			default:
				debug_printline(0xffffffff, "You have failed!");
				break;
			}
		}

		template <typename W>
		void operator() (const W & w)
		{
			debug_printline(0xffffffff, "You have failed!");
		}
	};

	void printClick(engine::Entity id)
	{
		if (components.contains(id))
		{
			components.call(id, PrintType{});
		}
	}
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

		auto game_renderswitch = engine::Entity::create();
		gameplay::ui::post_add_renderswitch(game_renderswitch, engine::hid::Input::Button::KEY_F5);
		gameplay::ui::post_bind("debug", game_renderswitch, -5);
		gameplay::ui::post_bind("game", game_renderswitch, -5);

		auto game_renderhover = engine::Entity::create();
		gameplay::ui::post_add_renderhover(game_renderhover);
		gameplay::ui::post_bind("debug", game_renderhover, 5);
		gameplay::ui::post_bind("game", game_renderhover, 5);

		auto game_renderselect = engine::Entity::create();
		components.emplace<Selector>(game_renderselect);
		gameplay::ui::post_add_renderselect(game_renderselect);
		gameplay::ui::post_bind("debug", game_renderselect, 5);
		gameplay::ui::post_bind("game", game_renderselect, 5);
	}

	void destroy()
	{

	}

	void update()
	{
		// adding workstuff
		{
			engine::Entity worker_args;
			while (queue_workers.try_pop(worker_args))
			{
				components.emplace<Worker>(
					worker_args,
					worker_args);
			}

			std::tuple<engine::Entity, WorkstationType> workstation_args;
			while (queue_workstations.try_pop(workstation_args))
			{
				components.emplace<Workstation>(
					std::get<0>(workstation_args),
					std::get<0>(workstation_args),
					std::get<1>(workstation_args));
			}
		}

		// commands
		{
			std::tuple<engine::Entity, engine::Command> command_args0;
			while (queue_commands0.try_pop(command_args0))
			{
				components.update(std::get<0>(command_args0), std::make_tuple(std::get<1>(command_args0), 0));
			}
			std::tuple<engine::Entity, engine::Command, engine::Entity> command_args1;
			while (queue_commands1.try_pop(command_args1))
			{
				components.update(std::get<0>(command_args1), std::make_tuple(std::get<1>(command_args1), std::get<2>(command_args1)));
			}
		}

		// update
		for (auto & camera : components.get<FreeCamera>())
		{
			camera.update();
		}
	}

	void post_command(engine::Entity entity, engine::Command command)
	{
		const auto res = queue_commands0.try_emplace(entity, command);
		debug_assert(res);
	}

	void post_command(engine::Entity entity, engine::Command command, engine::Entity arg1)
	{
		const auto res = queue_commands1.try_emplace(entity, command, arg1);
		debug_assert(res);
	}

	void post_add_workstation(engine::Entity entity, WorkstationType type)
	{
		const auto res = queue_workstations.try_emplace(entity, type);
		debug_assert(res);
	}
	void post_add_worker(engine::Entity entity)
	{
		const auto res = queue_workers.try_emplace(entity);
		debug_assert(res);
	}
}
}
