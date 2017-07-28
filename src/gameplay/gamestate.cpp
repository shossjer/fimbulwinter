
#include "gamestate.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/level_placeholder.hpp>
#include <gameplay/ui.hpp>

#include <utility>

namespace
{
	void entityClick(engine::Entity id);

	struct CameraActivator
	{
		engine::Entity camera;

		CameraActivator(engine::Entity camera) : camera(camera) {}

		void operator = (std::pair<engine::Command, utility::any> && args)
		{
			switch (args.first)
			{
			case engine::Command::CONTEXT_CHANGED:
				debug_assert(!args.second.has_value());
				debug_printline(0xffffffff, "Switching to camera: ", camera);
				engine::graphics::viewer::set_active_3d(camera);
				break;
			default:
				debug_printline(0xffffffff, "CameraActivator: Unknown command: ", static_cast<int>(args.first));
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
		bool elevate_down = false;
		bool elevate_up = false;

		core::maths::Quaternionf rotation = {1.f, 0.f, 0.f, 0.f};

		FreeCamera(engine::Entity camera) : camera(camera) {}

		void operator = (std::pair<engine::Command, utility::any> && args)
		{
			switch (args.first)
			{
			case engine::Command::MOVE_LEFT_DOWN:
				debug_assert(!args.second.has_value());
				move_left = true;
				break;
			case engine::Command::MOVE_LEFT_UP:
				debug_assert(!args.second.has_value());
				move_left = false;
				break;
			case engine::Command::MOVE_RIGHT_DOWN:
				debug_assert(!args.second.has_value());
				move_right = true;
				break;
			case engine::Command::MOVE_RIGHT_UP:
				debug_assert(!args.second.has_value());
				move_right = false;
				break;
			case engine::Command::MOVE_DOWN_DOWN:
				debug_assert(!args.second.has_value());
				move_down = true;
				break;
			case engine::Command::MOVE_DOWN_UP:
				debug_assert(!args.second.has_value());
				move_down = false;
				break;
			case engine::Command::MOVE_UP_DOWN:
				debug_assert(!args.second.has_value());
				move_up = true;
				break;
			case engine::Command::MOVE_UP_UP:
				debug_assert(!args.second.has_value());
				move_up = false;
				break;
			case engine::Command::TURN_LEFT_DOWN:
				debug_assert(!args.second.has_value());
				turn_left = true;
				break;
			case engine::Command::TURN_LEFT_UP:
				debug_assert(!args.second.has_value());
				turn_left = false;
				break;
			case engine::Command::TURN_RIGHT_DOWN:
				debug_assert(!args.second.has_value());
				turn_right = true;
				break;
			case engine::Command::TURN_RIGHT_UP:
				debug_assert(!args.second.has_value());
				turn_right = false;
				break;
			case engine::Command::TURN_DOWN_DOWN:
				debug_assert(!args.second.has_value());
				turn_down = true;
				break;
			case engine::Command::TURN_DOWN_UP:
				debug_assert(!args.second.has_value());
				turn_down = false;
				break;
			case engine::Command::TURN_UP_DOWN:
				debug_assert(!args.second.has_value());
				turn_up = true;
				break;
			case engine::Command::TURN_UP_UP:
				debug_assert(!args.second.has_value());
				turn_up = false;
				break;
			case engine::Command::ROLL_LEFT_DOWN:
				debug_assert(!args.second.has_value());
				roll_left = true;
				break;
			case engine::Command::ROLL_LEFT_UP:
				debug_assert(!args.second.has_value());
				roll_left = false;
				break;
			case engine::Command::ROLL_RIGHT_DOWN:
				debug_assert(!args.second.has_value());
				roll_right = true;
				break;
			case engine::Command::ROLL_RIGHT_UP:
				debug_assert(!args.second.has_value());
				roll_right = false;
				break;
			case engine::Command::ELEVATE_DOWN_DOWN:
				debug_assert(!args.second.has_value());
				elevate_down = true;
				break;
			case engine::Command::ELEVATE_DOWN_UP:
				debug_assert(!args.second.has_value());
				elevate_down = false;
				break;
			case engine::Command::ELEVATE_UP_DOWN:
				debug_assert(!args.second.has_value());
				elevate_up = true;
				break;
			case engine::Command::ELEVATE_UP_UP:
				debug_assert(!args.second.has_value());
				elevate_up = false;
				break;
			default:
				debug_printline(0xffffffff, "FreeCamera: Unknown command: ", static_cast<int>(args.first));
			}
		}

		void update()
		{
			const float rotation_speed = make_radian(core::maths::degreef{1.f/2.f}).get();
			if (turn_left)
				rotation *= core::maths::Quaternionf{std::cos(rotation_speed), 0.f, std::sin(rotation_speed), 0.f};
			if (turn_right)
				rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), 0.f, std::sin(-rotation_speed), 0.f};
			if (turn_up)
				rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), std::sin(-rotation_speed), 0.f, 0.f};
			if (turn_down)
				rotation *= core::maths::Quaternionf{std::cos(rotation_speed), std::sin(rotation_speed), 0.f, 0.f};
			if (roll_left)
				rotation *= core::maths::Quaternionf{std::cos(rotation_speed), 0.f, 0.f, std::sin(rotation_speed)};
			if (roll_right)
				rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), 0.f, 0.f, std::sin(-rotation_speed)};

			const float movement_speed = .5f;
			if (move_left)
				engine::physics::camera::update(
					camera,
					-rotation.axis_x() * movement_speed);
			if (move_right)
				engine::physics::camera::update(
					camera,
					rotation.axis_x() * movement_speed);
			if (move_up)
				engine::physics::camera::update(
					camera,
					-rotation.axis_z() * movement_speed);
			if (move_down)
				engine::physics::camera::update(
					camera,
					rotation.axis_z() * movement_speed);
			if (elevate_down)
				engine::physics::camera::update(
					camera,
					-rotation.axis_y() * movement_speed);
			if (elevate_up)
				engine::physics::camera::update(
					camera,
					rotation.axis_y() * movement_speed);

			engine::graphics::viewer::update(camera, engine::graphics::viewer::rotation{rotation});
		}
	};

	struct OverviewCamera
	{
		engine::Entity camera;

		int move_left = 0;
		int move_right = 0;
		int move_down = 0;
		int move_up = 0;

		OverviewCamera(engine::Entity camera) : camera(camera) {}

		void operator = (std::pair<engine::Command, utility::any> && args)
		{
			switch (args.first)
			{
			case engine::Command::MOVE_LEFT_DOWN:
				debug_assert(!args.second.has_value());
				move_left++;
				break;
			case engine::Command::MOVE_LEFT_UP:
				debug_assert(!args.second.has_value());
				move_left--;
				break;
			case engine::Command::MOVE_RIGHT_DOWN:
				debug_assert(!args.second.has_value());
				move_right++;
				break;
			case engine::Command::MOVE_RIGHT_UP:
				debug_assert(!args.second.has_value());
				move_right--;
				break;
			case engine::Command::MOVE_DOWN_DOWN:
				debug_assert(!args.second.has_value());
				move_down++;
				break;
			case engine::Command::MOVE_DOWN_UP:
				debug_assert(!args.second.has_value());
				move_down--;
				break;
			case engine::Command::MOVE_UP_DOWN:
				debug_assert(!args.second.has_value());
				move_up++;
				break;
			case engine::Command::MOVE_UP_UP:
				debug_assert(!args.second.has_value());
				move_up--;
				break;
			default:
				debug_printline(0xffffffff, "OverviewCamera: Unknown command: ", static_cast<int>(args.first));
			}
		}

		void update()
		{
			if (move_left > 0)
				engine::physics::camera::update(
					camera,
					Vector3f{-1.f, 0.f, 0.f});
			if (move_right > 0)
				engine::physics::camera::update(
					camera,
					Vector3f{1.f, 0.f, 0.f});
			if (move_up > 0)
				engine::physics::camera::update(
					camera,
					Vector3f{0.f, 0.f, -1.f});
			if (move_down > 0)
				engine::physics::camera::update(
					camera,
					Vector3f{0.f, 0.f, 1.f});
		}
	};

	struct Selector
	{
		void operator = (std::pair<engine::Command, utility::any> && args)
		{
			switch (args.first)
			{
			case engine::Command::RENDER_SELECT:
				entityClick(utility::any_cast<engine::Entity>(args.second));
				break;
			default:
				debug_printline(0xffffffff, "Selector: Unknown command: ", static_cast<int>(args.first));
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
			engine::graphics::renderer::post_add_bar(this->bar, engine::graphics::data::Bar{
				to_xyz(this->top.get_column<3>()) + Vector3f{ 0.f, .5f, 0.f }, progress});
		}

	public:

		bool isBusy()
		{
			return this->worker != engine::Entity::null();
		}

		void start()
		{
			engine::animation::update(this->worker, engine::animation::action{"work", true});

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
			engine::animation::update(this->worker, engine::animation::action{"idle", true});

			engine::graphics::renderer::post_remove(this->boardModel);
			this->boardModel = engine::Entity::null();

			engine::graphics::renderer::post_remove(this->bar);
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
		std::array<FreeCamera, 1>,
		std::array<OverviewCamera, 1>,
		std::array<Selector, 1>,
		std::array<Worker, 10>,
		std::array<Workstation, 20>
	>
	components;

	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Command, utility::any>, 100> queue_commands;

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
		core::maths::Vector3f translation;
		core::maths::Quaternionf rotation;
		core::maths::Vector3f scale;
		decompose(s.front, translation, rotation, scale);
		engine::physics::post_update_transform(w.id, engine::transform_t{translation, rotation});
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
		components.emplace<OverviewCamera>(game_camera, game_camera);

		engine::physics::camera::add(debug_camera, debug_camera_pos, false);
		engine::physics::camera::add(game_camera, game_camera_pos, true);

		engine::graphics::viewer::add(
				debug_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ 1.f, 0.f, 0.f, 0.f },
					debug_camera_pos});
		engine::graphics::viewer::add(
				game_camera,
				engine::graphics::viewer::camera{
					core::maths::Quaternionf{ std::cos(make_radian(core::maths::degreef{-40.f/2.f}).get()), std::sin(make_radian(core::maths::degreef{-40.f/2.f}).get()), 0.f, 0.f },
					game_camera_pos});
		engine::graphics::viewer::set_active_3d(game_camera);

		auto bordercontrol = engine::Entity::create();
		gameplay::ui::post_add_bordercontrol(bordercontrol, game_camera);
		auto flycontrol = engine::Entity::create();
		gameplay::ui::post_add_flycontrol(flycontrol, debug_camera);
		auto pancontrol = engine::Entity::create();
		gameplay::ui::post_add_pancontrol(pancontrol, game_camera);
		gameplay::ui::post_bind("debug", flycontrol, 0);
		gameplay::ui::post_bind("game", pancontrol, 0);
		gameplay::ui::post_bind("game", bordercontrol, 0);

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
			std::tuple<engine::Entity, engine::Command, utility::any> command_args;
			while (queue_commands.try_pop(command_args))
			{
				components.update(std::get<0>(command_args), std::make_pair(std::get<1>(command_args), std::move(std::get<2>(command_args))));
			}
		}

		// update
		for (auto & camera : components.get<FreeCamera>())
		{
			camera.update();
		}
		for (auto & camera : components.get<OverviewCamera>())
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
		post_command(entity, command, utility::any{});
	}

	void post_command(engine::Entity entity, engine::Command command, utility::any && data)
	{
		const auto res = queue_commands.try_emplace(entity, command, std::move(data));
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
