
#include "gamestate.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>

#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/level_placeholder.hpp>
#include <gameplay/ui.hpp>

namespace
{
	void entityClick(engine::Entity id);

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
				engine::physics::camera::update(
					camera,
					Vector3f{-1.f, 0.f, 0.f});
			if (move_right)
				engine::physics::camera::update(
					camera,
					Vector3f{1.f, 0.f, 0.f});
			if (move_up)
				engine::physics::camera::update(
					camera,
					Vector3f{0.f, 0.f, -1.f});
			if (move_down)
				engine::physics::camera::update(
					camera,
					Vector3f{0.f, 0.f, 1.f});
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
				entityClick(std::get<1>(args));
				break;
			default:
				debug_printline(0xffffffff, "Selector: Unknown command: ", static_cast<int>(std::get<0>(args)));
			}
		}
	};

	struct Storehouse
	{
	private:
		unsigned int carrotsRaw;
		unsigned int carrotsChopped;

	public:

		Storehouse()
			: carrotsRaw(10)
			, carrotsChopped(0)
		{
		}

	public:
		// return a carrot if available
		bool checkoutRawCarrot()
		{
			if (this->carrotsRaw <= 0)
				return false;

			this->carrotsRaw--;
			return true;
		}

		void checkinChoppedCarrot() { this->carrotsChopped++; }

		void print()
		{
			debug_printline(0xffffffff, "Carrot status - raw: ", this->carrotsRaw, " chopped: ", this->carrotsChopped);
		}
	} storage;

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
		Matrix4x4f front;
		Matrix4x4f top;
		engine::Entity worker;

	private:
		engine::Entity boardModel;
		double progress;
		engine::Entity bar;

	public:
		Workstation(
			engine::Entity id,
			gameplay::gamestate::WorkstationType type,
			Matrix4x4f front,
			Matrix4x4f top)
			: id(id)
			, type(type)
			, front(front)
			, top(top)
			, worker(engine::Entity::null())
			, boardModel(engine::Entity::null())
		{
		}

	private:

		void barUpdate(const float progress)
		{
			engine::graphics::renderer::add(this->bar, engine::graphics::data::Bar{
				to_xyz(this->top.get_column<3>()) + Vector3f{ 0.f, .5f, 0.f }, progress});
		}

	public:

		bool isBusy()
		{
			return this->worker != engine::Entity::null();
		}

		void start()
		{
			if (this->boardModel!= engine::Entity::null()) return;

			if (!storage.checkoutRawCarrot())
			{
				debug_printline(0xffffffff, "No more raw carrots to chopp!");
				return;
			}

			storage.print();

			this->boardModel = engine::Entity::create();
			gameplay::level::load(this->boardModel, "board", this->top);

			const auto pos = to_xyz(this->front.get_column<3>());
			this->bar = engine::Entity::create();
			barUpdate(0.f);
		}

		void cleanup()
		{
			engine::graphics::renderer::remove(this->boardModel);
			this->boardModel = engine::Entity::null();

			engine::graphics::renderer::remove(this->bar);
			this->bar = engine::Entity::null();
		}

		void update()
		{
			if (this->worker == engine::Entity::null())
				return;

			if (this->boardModel == engine::Entity::null())
				return;

			this->progress += (1000./50.);

			barUpdate(static_cast<float>(this->progress / (4. * 1000.)));

			if (this->progress < (4. * 1000.))
				return;

			// carrot is finished! either stop working or auto checkout a new carrot...

			this->progress = 0.;

			storage.checkinChoppedCarrot();
			storage.print();

			cleanup();
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
	core::container::CircleQueueSRMW<
		std::tuple
		<
			engine::Entity,
			gameplay::gamestate::WorkstationType,
			Matrix4x4f,
			Matrix4x4f
		>,
		100> queue_workstations;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_workers;

	void move_to_workstation(Worker & w, Workstation & s)
	{
		// clear prev. station if any
		if (w.workstation != engine::Entity::null())
		{
			components.get<Workstation>(w.workstation).worker = engine::Entity::null();
		}
		w.workstation = s.id;
		s.worker = w.id;

		s.start();

		// move the worker
		engine::graphics::renderer::update(
				w.id,
				engine::graphics::data::ModelviewMatrix{ s.front });
	}

	// manages player interaction (clicking) with entities.
	// whatever happends when something gets clicked, this one knows it..
	struct PlayerInteraction
	{
	private:
		engine::Entity selectedWorker = engine::Entity::null();

	public:
		// performs whatever should be done when a Worker is clicked
		void operator() (Worker & w)
		{
			debug_printline(0xffffffff, "You have clicked worker!");
			this->selectedWorker = w.id;
		}

		// performs whatever should be done when a Station is clicked
		void operator() (Workstation & s)
		{
			if (this->selectedWorker == engine::Entity::null())
				return;

			if (s.isBusy())
			{
				if (s.worker == selectedWorker)
				{
					debug_printline(0xffffffff, "I'm already working as fast as I can!");
					return;
				}
				debug_printline(0xffffffff, "The station is busy!");
				return;
			}

			// assign worker to station, clears prev. if any.
			move_to_workstation(components.get<Worker>(this->selectedWorker), s);
		}

		template <typename W>
		void operator() (const W & w)
		{
			debug_printline(0xffffffff, "You have failed!");
		}

	} playerInteraction;

	void entityClick(engine::Entity id)
	{
		if (components.contains(id))
		{
			components.call(id, playerInteraction);
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
		Vector3f debug_camera_pos{ 0.f, 4.f, 0.f };
		Vector3f game_camera_pos{ 0.f, 7.f, 5.f };

		components.emplace<FreeCamera>(debug_camera, debug_camera);
		components.emplace<FreeCamera>(game_camera, game_camera);

		engine::physics::camera::add(debug_camera, debug_camera_pos, false);
		engine::physics::camera::add(game_camera, game_camera_pos, true);

		engine::graphics::viewer::add(
				debug_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ 0.766f, 0.643f, 0.f, 0.f },
					debug_camera_pos});
		engine::graphics::viewer::add(
				game_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ std::cos(make_radian(core::maths::degreef{40.f/2.f}).get()), std::sin(make_radian(core::maths::degreef{40.f/2.f}).get()), 0.f, 0.f },
					game_camera_pos});
		engine::graphics::viewer::set_active_3d(game_camera);

		auto flycontrol = engine::Entity::create();
		gameplay::ui::post_add_flycontrol(flycontrol, debug_camera);
		auto pancontrol = engine::Entity::create();
		gameplay::ui::post_add_pancontrol(pancontrol, game_camera);
		gameplay::ui::post_bind("debug", flycontrol, 0);
		gameplay::ui::post_bind("game", pancontrol, 0);

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

			std::tuple<engine::Entity, WorkstationType, Matrix4x4f, Matrix4x4f> workstation_args;
			while (queue_workstations.try_pop(workstation_args))
			{
				components.emplace<Workstation>(
					std::get<0>(workstation_args),
					std::get<0>(workstation_args),
					std::get<1>(workstation_args),
					std::get<2>(workstation_args),
					std::get<3>(workstation_args));
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

		for (auto & station : components.get<Workstation>())
		{
			station.update();
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

	void post_add_workstation(
			engine::Entity entity,
			WorkstationType type,
			Matrix4x4f front,
			Matrix4x4f top)
	{
		const auto res = queue_workstations.try_emplace(entity, type, front, top);
		debug_assert(res);
	}
	void post_add_worker(engine::Entity entity)
	{
		const auto res = queue_workers.try_emplace(entity);
		debug_assert(res);
	}
}
}
