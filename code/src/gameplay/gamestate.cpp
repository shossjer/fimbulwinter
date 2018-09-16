
#include "gamestate.hpp"
#include "gamestate_gui.hpp"
#include "gamestate_models.hpp"

#include "core/JsonStructurer.hpp"
#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/maths/algorithm.hpp>
#include "core/PngStructurer.hpp"

#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include "engine/hid/ui.hpp"
#include "engine/gui/gui.hpp"
#include <engine/physics/physics.hpp>
#include "engine/replay/writer.hpp"
#include "engine/resource/reader.hpp"

#include "gameplay/commands.hpp"
#include "gameplay/component/CameraActivator.hpp"
#include "gameplay/component/FreeCamera.hpp"
#include "gameplay/component/OverviewCamera.hpp"
#include <gameplay/debug.hpp>
#include <gameplay/factory.hpp>
#include "gameplay/recipes.hpp"
#include "gameplay/roles.hpp"
#include "gameplay/skills.hpp"

#include "utility/string.hpp"

#include <algorithm>
#include <fstream>
#include <utility>


namespace
{
	using CameraActivator = gameplay::component::CameraActivator;
	using FreeCamera = gameplay::component::FreeCamera;
	using OverviewCamera = gameplay::component::OverviewCamera;

	using namespace gameplay::gamestate;

	template<typename T>
	T & access_component(const engine::Entity entity);

	// update "profile" of entity if it is currently shown (will not set entity as selected).
	void profile_update(const engine::Entity entity);

	std::unordered_map<engine::Asset, RecipeIngredient> ingredient_graph;

	struct Selector
	{
		engine::Entity highlighted_entity = engine::Entity::null();
		engine::Entity pressed_entity = engine::Entity::null();
		engine::Entity selected_entity = engine::Entity::null();
		engine::Entity targeted_entity = engine::Entity::null();

		void highlight(engine::Entity entity);
		void lowlight(engine::Entity entity);
		void select(engine::Entity entity);
		void deselect(engine::Entity entity);

		void translate(engine::Command command, utility::any && data);
	};

	struct Preparation
	{
		const gameplay::Recipe * recipe;

		int time_remaining;
		int number_of_stacks;
	};

	struct Kitchen
	{
		gameplay::Recipes recipes;
		gameplay::Roles roles;
		gameplay::Skills skills;

		core::container::Collection
		<
			engine::Entity,
			200,
			std::array<Preparation, 100>,
			std::array<Preparation, 1>
		> tables;

		void init_recipes(core::JsonStructurer && s)
		{
			s.read(recipes);

			debug_printline("recipes:");
			for (int i = 0; i < recipes.size(); i++)
			{
				debug_printline("name = \"", recipes.get(i).name, "\"");
				if (!recipes.get(i).ingredients.empty())
				{
					for (int j = 0; j < recipes.get(i).ingredients.size(); j++)
					{
						debug_printline(recipes.get(i).ingredients[j].quantity, "x ", recipes.get(i).ingredients[j].name);
					}
				}
				else
				{
					debug_printline("\"", recipes.get(i).name, "\" has no ingredients");
				}
			}
		}

		void init_roles(core::JsonStructurer && s)
		{
			s.read(roles);

			debug_printline("classes:");
			for (int i = 0; i < roles.size(); i++)
			{
				debug_printline("name = \"", roles.get(i).name, "\"");
			}
		}

		void init_skills(core::JsonStructurer && s)
		{
			s.read(skills);

			debug_printline("skills:");
			for (int i = 0; i < skills.size(); i++)
			{
				debug_printline("name = \"", skills.get(i).name, "\", type = \"", skills.get(i).type, "\"");
			}
		}

		std::vector<const gameplay::Recipe *> get_available_recipes() const
		{
			std::vector<const gameplay::Recipe *> available_recipes;

			std::vector<int> ingredient_counts(recipes.size(), 0);
			for (int i = 0; i < recipes.size(); i++)
			{
				// a raw ingredient does not have ingredients
				if (recipes.get(i).ingredients.empty())
				{
					// it does not make sense to have a time if the ingredient is raw
					debug_assert(!recipes.get(i).time.has_value());

					ingredient_counts[i] = (std::numeric_limits<int>::max)();
				}
			}
			for (const auto & preparation : tables.get<Preparation>())
			{
				if (preparation.time_remaining > 0)
					continue;

				// there should be at least one stack
				debug_assert(preparation.number_of_stacks > 0);

				const int i = recipes.index(*preparation.recipe);
				ingredient_counts[i] += preparation.number_of_stacks;
			}

			for (int i = 0; i < recipes.size(); i++)
			{
				if (!recipes.get(i).ingredients.empty())
				{
					bool is_available = true;
					for (int j = 0; j < recipes.get(i).ingredients.size(); j++)
					{
						const int index = recipes.find(recipes.get(i).ingredients[j].name);
						debug_assert(index >= 0);

						const int need = recipes.get(i).ingredients[j].quantity;
						const int have = ingredient_counts[index];
						if (have < need)
						{
							is_available = false;
							break;
						}
					}
					if (is_available)
					{
						available_recipes.push_back(&recipes.get(i));
					}
				}
			}

			return available_recipes;
		}

		bool is_empty(engine::Entity table) const
		{
			return !tables.contains(table);
		}

		bool has_preparation_in_progress(engine::Entity table) const
		{
			if (!tables.contains<Preparation>(table))
				return false;

			const auto & preparation = tables.get<Preparation>(table);
			return preparation.time_remaining > 0;
		}

		Preparation& prepare(engine::Entity table, const gameplay::Recipe & recipe)
		{
			debug_assert(is_empty(table));

			for (int j = 0; j < recipe.ingredients.size(); j++)
			{
				const int index = recipes.find(recipe.ingredients[j].name);
				if (recipes.get(index).ingredients.empty())
					continue; // raw ingredient

				int quantity = recipe.ingredients[j].quantity;
				while (quantity > 0)
				{
					for (auto & preparation : tables.get<Preparation>())
					{
						if (preparation.time_remaining > 0)
							continue;
						if (preparation.recipe->name != recipe.ingredients[j].name)
							continue;

						debug_assert(preparation.number_of_stacks > 0);
						if (preparation.number_of_stacks <= quantity)
						{
							quantity -= preparation.number_of_stacks;
							tables.remove(tables.get_key(preparation));
						}
						else
						{
							preparation.number_of_stacks -= quantity;
							quantity = 0;
						}
						break;
					}
				}
			}
			return tables.emplace<Preparation>(table, &recipe, recipe.time.value_or(0) * 50, recipe.amount.value_or(1));
		}
	} kitchen;

	struct Worker
	{
		engine::Entity workstation = engine::Entity::null();

		std::vector<double> skills;

		void clear_skills(const gameplay::Skills & skills)
		{
			this->skills.resize(skills.size(), 0.);
		}
		void add_skill(int index, double amount)
		{
			skills[index] += amount;
		}

		double score_role(const gameplay::Role & role) const
		{
			using std::begin;
			using std::end;

			std::vector<double> my_normalized_skills = skills;
			const double my_sum = std::accumulate(begin(my_normalized_skills), end(my_normalized_skills), 0.);
			if (my_sum != 0.)
			{
				for (int i = 0; i < my_normalized_skills.size(); i++)
				{
					my_normalized_skills[i] /= my_sum;
				}
			}

			std::vector<double> role_normalized_skills(skills.size(), role.default_weight);
			for (const auto & skill_weight : role.skill_weights)
			{
				const int index = kitchen.skills.find(skill_weight.name);
				debug_assert(index >= 0);
				role_normalized_skills[index] = skill_weight.weight;
			}
			const double role_sum = std::accumulate(begin(role_normalized_skills), end(role_normalized_skills), 0.);
			if (role_sum != 0.)
			{
				for (int i = 0; i < role_normalized_skills.size(); i++)
				{
					role_normalized_skills[i] /= role_sum;
				}
			}

			double diff = 0.;
			for (int i = 0; i < my_normalized_skills.size(); i++)
			{
				diff += std::abs(my_normalized_skills[i] - role_normalized_skills[i]);
			}
			return diff;
		}

		void compute_role(const gameplay::Roles & roles)
		{
			std::vector<double> scores;
			std::vector<int> indices;
			for (int i = 0; i < roles.size(); i++)
			{
				scores.push_back(score_role(roles.get(i)));
				indices.push_back(i);
			}
			std::sort(std::begin(indices), std::end(indices), [&](int a, int b){ return scores[a] < scores[b]; });

			debug_printline("best to worst matching classes:");
			for (int i = 0; i < roles.size(); i++)
			{
				debug_printline("\"", roles.get(indices[i]).name, "\" = ", static_cast<int>((2. - scores[indices[i]]) / 2. * 100.), "%");
			}
		}
	};

	struct Workstation
	{
		gameplay::gamestate::WorkstationType type;
		core::maths::Matrix4x4f front;
		core::maths::Matrix4x4f top;
		engine::Entity worker = engine::Entity::null();

	private:
		engine::Entity boardModel;
		engine::Entity bar;

	public:
		Workstation(
			gameplay::gamestate::WorkstationType type,
			core::maths::Matrix4x4f front,
			core::maths::Matrix4x4f top)
			: type(type)
			, front(front)
			, top(top)
			, boardModel(engine::Entity::create())
			, bar(engine::Entity::create())
		{}

	public:
		bool isBusy() const
		{
			return worker != engine::Entity::null();
		}

		void start(const Preparation & preparation)
		{
			debug_assert(preparation.time_remaining > 0);

			engine::animation::post_update_action(worker, engine::animation::action{"work", true});

			const int remaining_time = preparation.time_remaining;
			const int total_time = preparation.recipe->time.value_or(0) * 50;
			if (remaining_time == total_time)
			{
				gameplay::create_board(boardModel, top);
				barUpdate(0.f);
			}
		}

		void finish(const Preparation & preparation)
		{
			Worker & w = access_component<Worker>(worker);
			for (auto & skill_amount : preparation.recipe->skill_amounts)
			{
				const int index = kitchen.skills.find(skill_amount.name);
				debug_assert(index >= 0);
				w.add_skill(index, skill_amount.amount);
			}

			debug_printline("worker (", worker, ") skills:");
			for (int i = 0; i < kitchen.skills.size(); i++)
			{
				debug_printline("\"", kitchen.skills.get(i).name, "\" = ", w.skills[i]);
			}
			w.compute_role(kitchen.roles);
		}

		void update(Preparation & preparation)
		{
			if (worker == engine::Entity::null())
				return;

			if (preparation.time_remaining <= 0)
				return;

			preparation.time_remaining--;

			const auto progress_percentage = static_cast<float>(preparation.time_remaining) / static_cast<float>(preparation.recipe->time.value() * 50);
			barUpdate(progress_percentage);

			const Worker & w = access_component<Worker>(worker);
			profile_update(worker);

			if (preparation.time_remaining <= 0)
			{
				cleanup(preparation);
				finish(preparation);
			}
		}

	private:
		void cleanup(const Preparation & preparation)
		{
			engine::animation::post_update_action(worker, engine::animation::action{"idle", true});

			gameplay::destroy(boardModel);

			engine::graphics::renderer::post_remove(bar);
		}

		void barUpdate(float normalized_progress)
		{
			engine::graphics::renderer::post_add_bar(bar, engine::graphics::data::Bar{
					to_xyz(top.get_column<3>()) + core::maths::Vector3f{ 0.f, .5f, 0.f }, normalized_progress});
		}
	};

	struct
	{
		// entity of currently "profiled" object
		engine::Entity entity;

		void operator() (Worker & worker)
		{
			Player data{ "Chef Elzar" };
			data.skills.push_back(Player::Skill{ "Cutting" });
			data.skills.push_back(Player::Skill{ "Washing hands" });
			data.skills.push_back(Player::Skill{ "Potato" });
			engine::gui::post(engine::gui::MessageData{ encode_gui(data) });
		}

		template<typename T>
		void operator() (const T &)
		{}
	}
	profile_updater;

	struct GUIComponent
	{
		engine::Entity id;
	};

	struct GUIWindow
	{
		engine::Asset window;

		void translate(engine::Command command, utility::any && data)
		{
			switch (command)
			{
			case engine::command::BUTTON_DOWN_ACTIVE:
			case engine::command::BUTTON_DOWN_INACTIVE:
				debug_assert(!data.has_value());
				engine::gui::post(engine::gui::MessageVisibility{ window, engine::gui::MessageVisibility::TOGGLE });
				break;
			case engine::command::BUTTON_UP_ACTIVE:
			case engine::command::BUTTON_UP_INACTIVE:
				debug_assert(!data.has_value());
				break;
			default:
				debug_unreachable();
			}
		}
	};

	struct Option
	{
	};

	struct Loader
	{
		void translate(engine::Command command, utility::any && data)
		{
			debug_assert(command == engine::command::LOADER_FINISHED);
			debug_printline(gameplay::gameplay_channel, "WOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOWOW");
		}
	};

	core::container::Collection
	<
		engine::Entity,
		401,
		std::array<CameraActivator, 2>,
		std::array<FreeCamera, 1>,
		std::array<GUIComponent, 100>,
		std::array<GUIWindow, 10>,
		std::array<OverviewCamera, 1>,
		std::array<Selector, 1>,
		std::array<Worker, 10>,
		std::array<Workstation, 20>,
		std::array<Option, 40>,
		std::array<Loader, 1>
	>
	components;

	struct TryReadImage
	{
		core::graphics::Image & image;

		void operator () (core::PngStructurer && x)
		{
			x.read(image);
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("impossible to read, maybe");
		}
	};

	void data_callback_image(std::string name, engine::resource::reader::Structurer && structurer)
	{
		core::graphics::Image image;
		visit(TryReadImage{image}, std::move(structurer));

		debug_assert((name[0] == 'r' && name[1] == 'e' && name[2] == 's' && name[3] == '/'));
		debug_assert((name[name.size() - 4] == '.' && name[name.size() - 3] == 'p' && name[name.size() - 2] == 'n' && name[name.size() - 1] == 'g'));
		auto asset = engine::Asset(name.data() + 4, name.size() - 4 - 4);

		engine::graphics::renderer::post_register_texture(asset, std::move(image));
	}

	struct
	{
		std::vector<engine::Entity> entities;
		std::vector<engine::Entity> shown_entities;

		void init(const gameplay::Recipes & recipes)
		{
			entities.reserve(recipes.size());
			shown_entities.reserve(recipes.size());

			for (int i = 0; i < recipes.size(); i++)
			{
				engine::resource::reader::post_read(utility::to_string("res/", recipes.get(i).name, ".png"), data_callback_image);

				const engine::Entity entity = engine::Entity::create();
				entities.push_back(entity);
				components.emplace<Option>(entity);
			}
		}

		const gameplay::Recipe & get(const gameplay::Recipes & recipes, engine::Entity entity) const
		{
			debug_assert(recipes.size() == entities.size());

			auto maybe = std::find(entities.begin(), entities.end(), entity);
			debug_assert(maybe != entities.end());
			const int index = std::distance(entities.begin(), maybe);

			return recipes.get(index);
		}

		void hide()
		{
			for (auto entity : shown_entities)
			{
				engine::graphics::renderer::post_remove(entity);
			}
			shown_entities.clear();
		}

		void show(const gameplay::Recipes & recipes, const std::vector<const gameplay::Recipe *> & shown_recipes, const core::maths::Vector3f & center)
		{
			debug_assert(recipes.size() == entities.size());

			hide();

			for (int i = 0; i < shown_recipes.size(); i++)
			{
				const int recipe_index = recipes.index(*shown_recipes[i]);
				const auto entity = entities[recipe_index];
				debug_assert(std::find(shown_entities.begin(), shown_entities.end(), entity) == shown_entities.end());

				const float radius = 96.f;
				const float angle = static_cast<float>(i) / static_cast<float>(shown_recipes.size()) * 2.f * 3.14159265f - 3.14159265f / 2.f;

				core::maths::Matrix4x4f matrix = make_translation_matrix(
					center + core::maths::Vector3f(
						radius * std::cos(angle) - 32.f,
						radius * std::sin(angle) - 32.f,
						0.f));
				core::maths::Vector2f size(64.f, 64.f);

				engine::graphics::renderer::post_add_panel(
					entity,
					engine::graphics::data::ui::PanelT{
						matrix,
						size,
						engine::Asset(shown_recipes[i]->name)});
				engine::graphics::renderer::post_make_selectable(entity);
				shown_entities.push_back(entity);
			}
		}
	} recipes_ring;

	struct translate_command
	{
		engine::Command command;
		utility::any && data;

		template <typename T,
		          typename = decltype(std::declval<T>().translate(command, std::move(data)))>
		void impl(T & x, int)
		{
			x.translate(command, std::move(data));
		}
		template <typename T>
		void impl(T & x, ...)
		{
			debug_unreachable();
		}

		template <typename T>
		void operator () (T & x)
		{
			impl(x, 0);
		}
	};

	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Command, utility::any>, 100> queue_commands;

	core::container::CircleQueueSRMW<
		std::tuple
		<
			engine::Entity,
			gameplay::gamestate::WorkstationType,
			core::maths::Matrix4x4f,
			core::maths::Matrix4x4f
		>,
		100> queue_workstations;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_gui_components;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_workers;

	template<typename T>
	T & access_component(const engine::Entity entity)
	{
		return components.get<T>(entity);
	}

	void profile_update(const engine::Entity entity)
	{
		if (entity == profile_updater.entity)
		{
			components.call(entity, profile_updater);
		}
	}

	void move_to_workstation(Worker & w, engine::Entity we, Workstation & s, engine::Entity se)
	{
		// clear prev. station if any
		if (w.workstation != engine::Entity::null())
		{
			Workstation & prev_workstation = components.get<Workstation>(w.workstation);
			debug_assert(prev_workstation.worker == we);
			prev_workstation.worker = engine::Entity::null();
		}
		w.workstation = se;
		s.worker = we;

		// move the worker
		core::maths::Vector3f translation;
		core::maths::Quaternionf rotation;
		core::maths::Vector3f scale;
		decompose(s.front, translation, rotation, scale);
		engine::physics::post_update_transform(we, engine::transform_t{translation, rotation});
	}

	struct can_be_interacted_with
	{
		const Selector & selector;

		bool operator () (engine::Entity, const Worker &)
		{
			return true;
		}

		bool operator () (engine::Entity entity, const Workstation &)
		{
			const bool has_selected_worker = components.contains<Worker>(selector.selected_entity);
			if (!has_selected_worker)
				return false;

			const auto & available_recipes = kitchen.get_available_recipes();
			const bool has_available_recipes = !available_recipes.empty();
			if (!has_available_recipes)
				return false;

			const bool is_empty = kitchen.is_empty(entity);
			if (is_empty)
				return true;

			const bool has_preparation_in_progress = kitchen.has_preparation_in_progress(entity);
			if (has_preparation_in_progress)
				return true;

			return false;
		}

		bool operator () (engine::Entity, const GUIComponent &)
		{
			return true;
		}

		bool operator () (engine::Entity, const Option &)
		{
			return true;
		}

		template <typename T>
		bool operator () (engine::Entity, const T &)
		{
			return false;
		}
	};

	struct can_be_selected
	{
		const Selector & selector;

		bool operator () (const Worker &)
		{
			return true;
		}

		bool operator () (const GUIComponent &)
		{
			return true;
		}

		template <typename T>
		bool operator () (const T &)
		{
			return false;
		}
	};

	struct interact_with
	{
		Selector & selector;

		void operator () (engine::Entity entity, Workstation & x)
		{
			const bool has_selected_worker = components.contains<Worker>(selector.selected_entity);
			debug_assert(has_selected_worker);

			const auto & available_recipes = kitchen.get_available_recipes();
			const bool has_available_recipes = !available_recipes.empty();
			debug_assert(has_available_recipes);

			const bool is_empty = kitchen.is_empty(entity);
			const bool has_preparation_in_progress = kitchen.has_preparation_in_progress(entity);
			debug_assert((is_empty || has_preparation_in_progress));

			if (x.isBusy())
			{
				if (x.worker == selector.selected_entity)
				{
					debug_printline(gameplay::gameplay_channel, "I'm already working as fast as I can!");
					return;
				}
				debug_printline(gameplay::gameplay_channel, "The station is busy!");
				return;
			}

			if (has_preparation_in_progress)
			{
				Worker & worker = components.get<Worker>(selector.selected_entity);
				move_to_workstation(worker, selector.selected_entity, x, entity);

				auto & preparation = kitchen.tables.get<Preparation>(entity);
				if (preparation.time_remaining <= 0)
				{
					x.finish(preparation);
				}
				else
				{
					x.start(preparation);
				}
			}
			else
			{
				debug_printline("What would you want me to do? I can...");
				for (const auto& recipe : available_recipes)
				{
					debug_printline("make some ", recipe->name);
				}

				selector.targeted_entity = entity;
				recipes_ring.show(kitchen.recipes, available_recipes, {200.f, 200.f, 0.f});
			}
		}

		void operator () (engine::Entity entity, const Option &)
		{
			const bool has_selected_worker = components.contains<Worker>(selector.selected_entity);
			debug_assert(has_selected_worker);

			const bool has_targeted_workstation = components.contains<Workstation>(selector.targeted_entity);
			debug_assert(has_targeted_workstation);

			recipes_ring.hide();

			const gameplay::Recipe & recipe = recipes_ring.get(kitchen.recipes, entity);
			auto & preparation = kitchen.prepare(selector.targeted_entity, recipe);

			Worker & worker = components.get<Worker>(selector.selected_entity);
			Workstation & workstation = components.get<Workstation>(selector.targeted_entity);
			// assign worker to station, clears prev. if any.
			move_to_workstation(worker, selector.selected_entity, workstation, selector.targeted_entity);

			if (preparation.time_remaining <= 0)
			{
				workstation.finish(preparation);
			}
			else
			{
				workstation.start(preparation);
			}
		}

		template <typename T>
		void operator () (engine::Entity, const T &)
		{
			recipes_ring.hide();
		}
	};

	void Selector::highlight(engine::Entity entity)
	{
		engine::graphics::renderer::post_make_highlight(entity);
	}

	void Selector::lowlight(engine::Entity entity)
	{
		engine::graphics::renderer::post_make_dehighlight(entity);
	}

	void Selector::select(engine::Entity entity)
	{
		engine::graphics::renderer::post_make_select(entity);
	}

	void Selector::deselect(engine::Entity entity)
	{
		engine::graphics::renderer::post_make_deselect(entity);
	}

	void update_gui(engine::Entity entity, engine::gui::MessageInteraction::State state)
	{
		// check if entity is "gui" component
		engine::gui::post(engine::gui::MessageInteraction{ entity, state });
	}

	void Selector::translate(engine::Command command, utility::any && data)
	{
		switch (command)
		{
		case engine::command::RENDER_HIGHLIGHT:
		{
			engine::Entity entity = utility::any_cast<engine::Entity>(data);

			if (highlighted_entity == entity)
			{
				if (entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(entity, can_be_interacted_with{*this});
					if (!is_interactible)
					{
						lowlight(entity);
						update_gui(entity, engine::gui::MessageInteraction::LOWLIGHT);
						highlighted_entity = engine::Entity::null();
					}
				}
			}
			else
			{
				if (highlighted_entity != engine::Entity::null())
				{
					lowlight(highlighted_entity);
					update_gui(highlighted_entity, engine::gui::MessageInteraction::LOWLIGHT);
					highlighted_entity = engine::Entity::null();
				}
				if (entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(entity, can_be_interacted_with{*this});
					if (is_interactible)
					{
						highlight(entity);
						update_gui(entity, engine::gui::MessageInteraction::HIGHLIGHT);
						highlighted_entity = entity;
					}
				}
			}
			break;
		}
		case engine::command::RENDER_SELECT:
		{
			engine::Entity entity = utility::any_cast<engine::Entity>(data);

			update_gui(entity, engine::gui::MessageInteraction::PRESS);
			pressed_entity = entity;
			break;
		}
		case engine::command::RENDER_DESELECT:
		{
			engine::Entity entity = utility::any_cast<engine::Entity>(data);

			if (pressed_entity == entity)
			{
				if (entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(entity, can_be_interacted_with{*this});
					if (is_interactible)
					{
						const bool is_selectable = components.call(entity, can_be_selected{*this});
						if (is_selectable)
						{
							if (selected_entity == entity)
							{
								deselect(entity);
								update_gui(entity, engine::gui::MessageInteraction::RELEASE);
								selected_entity = engine::Entity::null();
							}
							else
							{
								if (selected_entity != engine::Entity::null())
								{
									deselect(selected_entity);
									update_gui(selected_entity, engine::gui::MessageInteraction::RELEASE);
									selected_entity = engine::Entity::null();
								}
								select(entity);
								update_gui(entity, engine::gui::MessageInteraction::PRESS);
								selected_entity = entity;
							}
						}
						components.call(entity, interact_with{*this});
					}
					else
					{
						if (selected_entity != engine::Entity::null())
						{
							deselect(selected_entity);
							update_gui(selected_entity, engine::gui::MessageInteraction::RELEASE);
							selected_entity = engine::Entity::null();
						}
						recipes_ring.hide();
					}
				}
				else
				{
					if (selected_entity != engine::Entity::null())
					{
						deselect(selected_entity);
						update_gui(selected_entity, engine::gui::MessageInteraction::RELEASE);
						selected_entity = engine::Entity::null();
					}
					recipes_ring.hide();
				}
			}
			else
			{
				if (selected_entity != engine::Entity::null())
				{
					deselect(selected_entity);
					update_gui(selected_entity, engine::gui::MessageInteraction::RELEASE);
					selected_entity = engine::Entity::null();
				}
				recipes_ring.hide();
			}
			break;
		}
		default:
			debug_printline(gameplay::gameplay_channel, "Selector: Unknown command: ", static_cast<int>(command));
		}
	}
}

namespace
{
	struct data_callback_recipes_handler
	{
		void operator () (core::JsonStructurer && s)
		{
			kitchen.init_recipes(std::move(s));
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("unknown format");
		}
	};

	struct data_callback_roles_handler
	{
		void operator () (core::JsonStructurer && s)
		{
			kitchen.init_roles(std::move(s));
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("unknown format");
		}
	};

	struct data_callback_skills_handler
	{
		void operator () (core::JsonStructurer && s)
		{
			kitchen.init_skills(std::move(s));
		}
		template <typename T>
		void operator () (T && x)
		{
			debug_fail("unknown format");
		}
	};

	void data_callback_recipes(std::string name, engine::resource::reader::Structurer && structurer)
	{
		utility::visit(data_callback_recipes_handler{}, std::move(structurer));

		recipes_ring.init(kitchen.recipes);
	}

	void data_callback_roles(std::string name, engine::resource::reader::Structurer && structurer)
	{
		utility::visit(data_callback_roles_handler{}, std::move(structurer));
	}

	void data_callback_skills(std::string name, engine::resource::reader::Structurer && structurer)
	{
		utility::visit(data_callback_skills_handler{}, std::move(structurer));
	}
}

namespace gameplay
{
namespace gamestate
{
	void create()
	{
		engine::hid::ui::post_add_context("debug");
		engine::hid::ui::post_add_context("game");
		engine::hid::ui::post_activate_context("game");

		auto debug_camera = engine::Entity::create();
		auto game_camera = engine::Entity::create();
		core::maths::Vector3f debug_camera_pos{ 0.f, 4.f, 0.f };
		core::maths::Vector3f game_camera_pos{ 0.f, 7.f, 5.f };

		components.emplace<FreeCamera>(debug_camera, debug_camera);
		components.emplace<OverviewCamera>(game_camera, game_camera);

		engine::physics::camera::add(debug_camera, debug_camera_pos, false);
		engine::physics::camera::add(game_camera, game_camera_pos, true);

		engine::graphics::viewer::post_add_frame("game", engine::graphics::viewer::dynamic{"root"});

		engine::graphics::viewer::post_add_projection("my-perspective-3d", engine::graphics::viewer::perspective{core::maths::make_degree(80.), .125, 128.});
		engine::graphics::viewer::post_add_projection("my-perspective-2d", engine::graphics::viewer::orthographic{-100., 100});

		engine::graphics::viewer::post_add_camera(
				debug_camera,
				engine::graphics::viewer::camera{
					"my-perspective-3d",
					"my-perspective-2d",
					core::maths::Quaternionf{ 1.f, 0.f, 0.f, 0.f },
					debug_camera_pos});
		engine::graphics::viewer::post_add_camera(
				game_camera,
				engine::graphics::viewer::camera{
					"my-perspective-3d",
					"my-perspective-2d",
					core::maths::Quaternionf{ std::cos(make_radian(core::maths::degreef{-40.f/2.f}).get()), std::sin(make_radian(core::maths::degreef{-40.f/2.f}).get()), 0.f, 0.f },
					game_camera_pos});
		engine::graphics::viewer::post_bind("game", game_camera);

		auto bordercontrol = engine::Entity::create();
		engine::hid::ui::post_add_bordercontrol(bordercontrol, game_camera);
		auto flycontrol = engine::Entity::create();
		engine::hid::ui::post_add_flycontrol(flycontrol, debug_camera);
		auto pancontrol = engine::Entity::create();
		engine::hid::ui::post_add_pancontrol(pancontrol, game_camera);
		engine::hid::ui::post_bind("debug", flycontrol, 0);
		engine::hid::ui::post_bind("game", pancontrol, 0);
		engine::hid::ui::post_bind("game", bordercontrol, 0);

		profile_updater.entity = engine::Entity::null();
		auto inventorycontrol = engine::Entity::create();
		engine::hid::ui::post_add_buttoncontrol(inventorycontrol, engine::hid::Input::Button::KEY_I);
		engine::hid::ui::post_bind("debug", inventorycontrol, 0);
		engine::hid::ui::post_bind("game", inventorycontrol, 0);
		components.emplace<GUIWindow>(inventorycontrol, "inventory");
		auto profilecontrol = engine::Entity::create();
		engine::hid::ui::post_add_buttoncontrol(profilecontrol, engine::hid::Input::Button::KEY_P);
		engine::hid::ui::post_bind("debug", profilecontrol, 0);
		engine::hid::ui::post_bind("game", profilecontrol, 0);
		components.emplace<GUIWindow>(profilecontrol, "profile");

		auto debug_switch = engine::Entity::create();
		auto game_switch = engine::Entity::create();

		components.emplace<CameraActivator>(debug_switch, "game", debug_camera);
		components.emplace<CameraActivator>(game_switch, "game", game_camera);

		engine::hid::ui::post_add_contextswitch(debug_switch, engine::hid::Input::Button::KEY_F1, "debug");
		engine::hid::ui::post_add_contextswitch(game_switch, engine::hid::Input::Button::KEY_F2, "game");
		engine::hid::ui::post_bind("debug", game_switch, -10);
		engine::hid::ui::post_bind("game", debug_switch, -10);

		auto game_renderswitch = engine::Entity::create();
		engine::hid::ui::post_add_renderswitch(game_renderswitch, engine::hid::Input::Button::KEY_F5);
		engine::hid::ui::post_bind("debug", game_renderswitch, -5);
		engine::hid::ui::post_bind("game", game_renderswitch, -5);

		auto selector = engine::Entity::create();
		components.emplace<Selector>(selector);

		auto game_renderhover = engine::Entity::create();
		engine::hid::ui::post_add_renderhover(game_renderhover, selector);
		engine::hid::ui::post_bind("debug", game_renderhover, 5);
		engine::hid::ui::post_bind("game", game_renderhover, 5);

		auto game_renderselect = engine::Entity::create();
		engine::hid::ui::post_add_renderselect(game_renderselect, selector);
		engine::hid::ui::post_bind("debug", game_renderselect, 5);
		engine::hid::ui::post_bind("game", game_renderselect, 5);

		// vvvv tmp vvvv
		gameplay::create_level(engine::Entity::create(), "level");

		// assign reaction structure to engine::gui
		engine::gui::post(engine::gui::MessageDataSetup{ encode_gui(Player{ "name",{ Player::Skill{ "name" } } }) });
		{
			Dish dish{ std::string{ "name" }, std::string{ "desc" } };
			engine::gui::post(engine::gui::MessageDataSetup{ encode_gui({ &dish, &dish, &dish, &dish }) });
		}

		// trigger first load of GUI
		engine::gui::post(engine::gui::MessageReload{});

		engine::resource::reader::post_read("recipes", data_callback_recipes);
		engine::resource::reader::post_read("classes", data_callback_roles);
		engine::resource::reader::post_read("skills", data_callback_skills);

		{
			Player data{ "Chef Elzar" };
			data.skills.push_back(Player::Skill{ "Cutting" });
			data.skills.push_back(Player::Skill{ "Washing hands" });
			data.skills.push_back(Player::Skill{ "Potato" });
			engine::gui::post(engine::gui::MessageData{ encode_gui(data) });
		}
	}

	void destroy()
	{

	}

	void update(int frame_count)
	{
		// adding workstuff
		{
			engine::Entity worker_args;
			while (queue_workers.try_pop(worker_args))
			{
				Worker& worker = components.emplace<Worker>(worker_args);
				worker.clear_skills(kitchen.skills);
			}

			std::tuple<engine::Entity, WorkstationType, core::maths::Matrix4x4f, core::maths::Matrix4x4f> workstation_args;
			while (queue_workstations.try_pop(workstation_args))
			{
				components.emplace<Workstation>(
					std::get<0>(workstation_args),
					std::get<1>(workstation_args),
					std::get<2>(workstation_args),
					std::get<3>(workstation_args));
			}

			engine::Entity gui_component;
			while (queue_gui_components.try_pop(gui_component))
			{
				components.emplace<GUIComponent>(gui_component, gui_component);
			}
		}

		// commands
		{
			std::tuple<engine::Entity, engine::Command, utility::any> command_args;
			while (queue_commands.try_pop(command_args))
			{
				engine::replay::post_add_command(frame_count, std::get<0>(command_args), std::get<1>(command_args), utility::any(std::get<2>(command_args)));

				debug_assert(std::get<0>(command_args) != engine::Entity::null());
				if (std::get<0>(command_args) != engine::Entity::null())
				{
					components.call(std::get<0>(command_args), translate_command{std::get<1>(command_args), std::move(std::get<2>(command_args))});
				}
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
			const auto entity = components.get_key(station);
			if (kitchen.tables.contains<Preparation>(entity))
			{
				auto & preparation = kitchen.tables.get<Preparation>(entity);
				station.update(preparation);
			}
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

	void post_gui(engine::Entity entity)
	{
		const auto res = queue_gui_components.try_emplace(entity);
		debug_assert(res);
	}

	void post_add_workstation(
			engine::Entity entity,
			WorkstationType type,
			core::maths::Matrix4x4f front,
			core::maths::Matrix4x4f top)
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
