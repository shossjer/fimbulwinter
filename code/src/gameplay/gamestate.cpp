
#include "gamestate.hpp"
#include "gamestate_models.hpp"

#include "core/JsonStructurer.hpp"
#include "core/container/CircleQueue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"
#include "core/maths/algorithm.hpp"
#include "core/PngStructurer.hpp"

#include "engine/animation/mixer.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/hid/ui.hpp"
#include "engine/physics/physics.hpp"
#include "engine/replay/writer.hpp"
#include "engine/resource/reader.hpp"

#include "gameplay/commands.hpp"
#include "gameplay/component/CameraActivator.hpp"
#include "gameplay/component/FreeCamera.hpp"
#include "gameplay/component/OverviewCamera.hpp"
#include "gameplay/debug.hpp"
#include "gameplay/factory.hpp"
#include "gameplay/recipes.hpp"
#include "gameplay/roles.hpp"
#include "gameplay/skills.hpp"

#include "utility/string.hpp"

#include <algorithm>
#include <fstream>
#include <utility>

debug_assets("debug", "default", "game", "my-perspective-2d", "my-perspective-3d");

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
			utility::static_storage<Preparation, 100>
		>
		tables;

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

		bool has_preparation(engine::Entity table) const
		{
			return tables.contains<Preparation>(table);
		}

		bool has_preparation_in_progress(engine::Entity table) const
		{
			if (!tables.contains<Preparation>(table))
				return false;

			const auto & preparation = tables.get<Preparation>(table);
			return preparation.time_remaining > 0;
		}

		const Preparation & get_preparation(engine::Entity table) const
		{
			debug_assert(has_preparation(table));

			return tables.get<Preparation>(table);
		}

		Preparation & prepare(engine::Entity table, const gameplay::Recipe & recipe)
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

		const gameplay::Role & compute_role(const gameplay::Roles & roles) const
		{
			debug_assert(roles.size() > 0);

			double best_score = score_role(roles.get(0));
			int best_index = 0;
			for (int i = 1; i < roles.size(); i++)
			{
				const double score = score_role(roles.get(i));
				if (score < best_score)
				{
					best_score = score;
					best_index = i;
				}
			}
			return roles.get(best_index);
		}

		void print_best_to_worst_roles(const gameplay::Roles & roles) const
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
			w.print_best_to_worst_roles(kitchen.roles);
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

			if (preparation.time_remaining <= 0)
			{
				cleanup(preparation);
				finish(preparation);
			}
		}

		void cleanup(const Preparation & preparation)
		{
			engine::animation::post_update_action(worker, engine::animation::action{"idle", true});

			gameplay::destroy(boardModel);

			engine::graphics::renderer::post_remove(bar);
		}

	private:
		void barUpdate(float normalized_progress)
		{
			engine::graphics::renderer::post_add_bar(bar, engine::graphics::data::Bar{
					to_xyz(top.get_column<3>()) + core::maths::Vector3f{ 0.f, .5f, 0.f }, normalized_progress});
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
		utility::static_storage<CameraActivator, 2>,
		utility::static_storage<FreeCamera, 1>,
		utility::static_storage<OverviewCamera, 1>,
		utility::static_storage<Selector, 1>,
		utility::heap_storage<Worker>,
		utility::heap_storage<Workstation>,
		utility::heap_storage<Option>,
		utility::static_storage<Loader, 1>
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
		const engine::Asset asset(name.substr(4, name.length() - 4 - 4));

		engine::graphics::renderer::post_register_texture(asset, std::move(image));
	}

	struct
	{
		std::vector<engine::Entity> recipe_entities;
		std::vector<engine::Entity> other_entities;

		std::vector<engine::Entity> shown_entities;

		void init(const gameplay::Recipes & recipes)
		{
			recipe_entities.reserve(recipes.size());
			other_entities.reserve(2);
			shown_entities.reserve(recipes.size());

			for (int i = 0; i < recipes.size(); i++)
			{
				engine::resource::reader::post_read(utility::to_string("res/", recipes.get(i).name, ".png"), data_callback_image);

				const engine::Entity entity = engine::Entity::create();
				recipe_entities.push_back(entity);
				components.emplace<Option>(entity);
			}

			for (int i = 0; i < 2; i++)
			{
				const engine::Entity entity = engine::Entity::create();
				other_entities.push_back(entity);
				components.emplace<Option>(entity);
			}
		}

		bool is_recipe(engine::Entity entity) const
		{
			auto maybe = std::find(recipe_entities.begin(), recipe_entities.end(), entity);
			return maybe != recipe_entities.end();
		}

		const gameplay::Recipe & get(const gameplay::Recipes & recipes, engine::Entity entity) const
		{
			debug_assert(recipes.size() == recipe_entities.size());

			auto maybe = std::find(recipe_entities.begin(), recipe_entities.end(), entity);
			debug_assert(maybe != recipe_entities.end());
			const int index = std::distance(recipe_entities.begin(), maybe);

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
			debug_assert(recipes.size() == recipe_entities.size());

			hide();

			for (int i = 0; i < shown_recipes.size(); i++)
			{
				const int recipe_index = recipes.index(*shown_recipes[i]);
				const auto entity = recipe_entities[recipe_index];
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

		int index_of_other(engine::Asset asset) const
		{
			constexpr engine::Asset assets[] = {engine::Asset("continue"), engine::Asset("trash")};

			auto maybe = std::find(std::begin(assets), std::end(assets), asset);
			debug_assert(maybe != std::end(assets));
			return std::distance(std::begin(assets), maybe);
		}

		std::string name_of_other(engine::Entity entity) const
		{
			constexpr const char * const names[] = {"continue", "trash"};

			auto maybe = std::find(other_entities.begin(), other_entities.end(), entity);
			debug_assert(maybe != other_entities.end());
			const int index = std::distance(other_entities.begin(), maybe);
			return names[index];
		}

		void show(const engine::Asset * const assets, int nassets, const core::maths::Vector3f & center)
		{
			hide();

			for (int i = 0; i < nassets; i++)
			{
				const int other_index = index_of_other(assets[i]);
				const auto entity = other_entities[other_index];

				const float radius = 96.f;
				const float angle = static_cast<float>(i) / static_cast<float>(nassets) * 2.f * 3.14159265f - 3.14159265f / 2.f;

				core::maths::Matrix4x4f matrix = make_translation_matrix(
					center + core::maths::Vector3f(
						radius * std::cos(angle) - 32.f,
						radius * std::sin(angle) - 32.f,
						0.f));
				core::maths::Vector2f size(64.f, 64.f);

				engine::graphics::renderer::post_add_panel(
					entity,
					engine::graphics::data::ui::PanelC{
						matrix,
						size,
						other_index == 0 ? 0xff00ffff : other_index == 1 ? 0xff00ff00 : other_index == 2 ? 0xff0000ff : 0xffcccccc});
				engine::graphics::renderer::post_make_selectable(entity);
				shown_entities.push_back(entity);
			}
		}
	} recipes_ring;

	struct get_tooltip
	{
		std::string operator () (engine::Entity entity, const Option &)
		{
			if (recipes_ring.is_recipe(entity))
			{
				const gameplay::Recipe & recipe = recipes_ring.get(kitchen.recipes, entity);
				return recipe.name;
			}
			else
			{
				return recipes_ring.name_of_other(entity);
			}
		}

		std::string operator () (engine::Entity entity, const Worker & x)
		{
			const auto & role = x.compute_role(kitchen.roles);
			return role.name;
		}

		std::string operator () (engine::Entity entity, const Workstation & x)
		{
			if (kitchen.is_empty(entity))
				return "empty";

			if (kitchen.has_preparation_in_progress(entity))
			{
				const auto & preparation = kitchen.get_preparation(entity);
				const double percentage_complete = static_cast<double>(preparation.recipe->time.value() * 50 - preparation.time_remaining) / static_cast<double>(preparation.recipe->time.value() * 50) * 100.;
				return utility::to_string(preparation.recipe->name, " ", static_cast<int>(percentage_complete), "%");
			}

			if (kitchen.has_preparation(entity))
			{
				const auto & preparation = kitchen.get_preparation(entity);
				return utility::to_string(preparation.number_of_stacks, "x ", preparation.recipe->name);
			}

			return "unknown workstation state";
		}

		template <typename X>
		std::string operator () (engine::Entity entity, const X &)
		{
			return utility::to_string("description for entity ", entity);
		}
	};

	struct Tooltip
	{
		engine::Entity frame = engine::Entity::create();
		engine::Entity label = engine::Entity::create();

		engine::Entity target = engine::Entity::null();
		int x;
		int y;

		engine::graphics::data::ui::PanelC build_frame() const
		{
			return {make_translation_matrix(core::maths::Vector3f(static_cast<float>(x), static_cast<float>(y), 90.f)),
			        {200.f, 16.f},
			        0xff000000};
		}
		engine::graphics::data::ui::Text build_label() const
		{
			return {make_translation_matrix(core::maths::Vector3f(static_cast<float>(x) + 1.f, static_cast<float>(y) + 15.f, 91.f)),
			        0xffffffff,
			        components.call(target, get_tooltip{})};
		}

		void display(engine::Entity entity, int x, int y)
		{
			if (target == entity)
			{
				if (target != engine::Entity::null())
				{
					engine::graphics::renderer::post_update_panel(frame, build_frame());
					engine::graphics::renderer::post_update_text(label, build_label());
				}
				return;
			}

			if (target != engine::Entity::null())
			{
				engine::graphics::renderer::post_remove(label);
				engine::graphics::renderer::post_remove(frame);
			}
			target = entity;
			this->x = x;
			this->y = y;

			if (target != engine::Entity::null())
			{
				engine::graphics::renderer::post_add_panel(frame, build_frame());
				engine::graphics::renderer::post_add_text(label, build_label());
			}
		}
	} tooltip;

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

	core::container::CircleQueueSRMW<engine::Entity, 100> queue_workers;

	template<typename T>
	T & access_component(const engine::Entity entity)
	{
		return components.get<T>(entity);
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

		bool operator () (engine::Entity entity, const Workstation & x)
		{
			const bool has_selected_worker = components.contains<Worker>(selector.selected_entity);
			if (!has_selected_worker)
				return false;

			const bool is_busy = x.isBusy() && x.worker != selector.selected_entity;
			if (is_busy)
				return false;

			const bool is_empty = kitchen.is_empty(entity);
			if (!is_empty)
				return true;

			const auto & available_recipes = kitchen.get_available_recipes();
			const bool has_available_recipes = !available_recipes.empty();
			if (!has_available_recipes)
				return false;

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

			const bool is_busy = x.isBusy() && x.worker != selector.selected_entity;
			debug_assert(!is_busy);

			const bool is_empty = kitchen.is_empty(entity);
			if (is_empty)
			{
				const auto & available_recipes = kitchen.get_available_recipes();
				const bool has_available_recipes = !available_recipes.empty();
				debug_assert(has_available_recipes);

				selector.targeted_entity = entity;
				recipes_ring.show(kitchen.recipes, available_recipes, {200.f, 200.f, 0.f});
				return;
			}

			const bool has_preparation_in_progress = kitchen.has_preparation_in_progress(entity);
			if (has_preparation_in_progress)
			{
				constexpr engine::Asset assets[] = { engine::Asset("continue"), engine::Asset("trash") };

				selector.targeted_entity = entity;
				recipes_ring.show(assets, 2, {200.f, 200.f, 0.f});
				return;
			}

			constexpr engine::Asset assets[] = { engine::Asset("trash") };

			selector.targeted_entity = entity;
			recipes_ring.show(assets, 1, {200.f, 200.f, 0.f});
		}

		void operator () (engine::Entity entity, const Option &)
		{
			const bool has_selected_worker = components.contains<Worker>(selector.selected_entity);
			debug_assert(has_selected_worker);

			const bool has_targeted_workstation = components.contains<Workstation>(selector.targeted_entity);
			debug_assert(has_targeted_workstation);

			recipes_ring.hide();

			if (recipes_ring.is_recipe(entity))
			{
				Worker & worker = components.get<Worker>(selector.selected_entity);
				Workstation & workstation = components.get<Workstation>(selector.targeted_entity);
				move_to_workstation(worker, selector.selected_entity, workstation, selector.targeted_entity);

				const gameplay::Recipe & recipe = recipes_ring.get(kitchen.recipes, entity);
				auto & preparation = kitchen.prepare(selector.targeted_entity, recipe);
				if (preparation.time_remaining <= 0)
				{
					workstation.finish(preparation);
				}
				else
				{
					workstation.start(preparation);
				}
			}
			else
			{
				switch (engine::Asset(recipes_ring.name_of_other(entity)))
				{
				case engine::Asset("continue"):
				{
					const bool has_preparation_in_progress = kitchen.has_preparation_in_progress(selector.targeted_entity);
					debug_assert(has_preparation_in_progress);

					Worker & worker = components.get<Worker>(selector.selected_entity);
					Workstation & workstation = components.get<Workstation>(selector.targeted_entity);
					move_to_workstation(worker, selector.selected_entity, workstation, selector.targeted_entity);

					auto & preparation = kitchen.tables.get<Preparation>(selector.targeted_entity);
					if (preparation.time_remaining <= 0)
					{
						workstation.finish(preparation);
					}
					else
					{
						workstation.start(preparation);
					}
					break;
				}
				case engine::Asset("trash"):
				{
					const bool is_empty = kitchen.is_empty(selector.targeted_entity);
					debug_assert(!is_empty);

					Worker & worker = components.get<Worker>(selector.selected_entity);
					Workstation & workstation = components.get<Workstation>(selector.targeted_entity);
					move_to_workstation(worker, selector.selected_entity, workstation, selector.targeted_entity);

					auto & preparation = kitchen.tables.get<Preparation>(selector.targeted_entity);
					if (preparation.time_remaining > 0)
					{
						workstation.cleanup(preparation);
					}
					kitchen.tables.remove(selector.targeted_entity);
					break;
				}
				default:
					debug_unreachable();
				}
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

	void Selector::translate(engine::Command command, utility::any && data)
	{
		switch (command)
		{
		case gameplay::command::RENDER_HIGHLIGHT:
		{
			const engine::graphics::renderer::SelectData select_data = utility::any_cast<engine::graphics::renderer::SelectData>(data);

			if (highlighted_entity == select_data.entity)
			{
				if (select_data.entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(select_data.entity, can_be_interacted_with{*this});
					if (!is_interactible)
					{
						lowlight(select_data.entity);
						highlighted_entity = engine::Entity::null();
					}
				}
			}
			else
			{
				if (highlighted_entity != engine::Entity::null())
				{
					lowlight(highlighted_entity);
					highlighted_entity = engine::Entity::null();
				}
				if (select_data.entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(select_data.entity, can_be_interacted_with{*this});
					if (is_interactible)
					{
						highlight(select_data.entity);
						highlighted_entity = select_data.entity;
					}
				}
			}
			tooltip.display(select_data.entity, select_data.cursor.x, select_data.cursor.y);
			break;
		}
		case gameplay::command::RENDER_SELECT:
		{
			const engine::graphics::renderer::SelectData select_data = utility::any_cast<engine::graphics::renderer::SelectData>(data);

			pressed_entity = select_data.entity;
			break;
		}
		case gameplay::command::RENDER_DESELECT:
		{
			const engine::graphics::renderer::SelectData select_data = utility::any_cast<engine::graphics::renderer::SelectData>(data);

			if (pressed_entity == select_data.entity)
			{
				if (select_data.entity != engine::Entity::null())
				{
					const bool is_interactible = components.call(select_data.entity, can_be_interacted_with{*this});
					if (is_interactible)
					{
						const bool is_selectable = components.call(select_data.entity, can_be_selected{*this});
						if (is_selectable)
						{
							if (selected_entity == select_data.entity)
							{
								deselect(select_data.entity);
								selected_entity = engine::Entity::null();
							}
							else
							{
								if (selected_entity != engine::Entity::null())
								{
									deselect(selected_entity);
									selected_entity = engine::Entity::null();
								}
								select(select_data.entity);
								selected_entity = select_data.entity;
							}
						}
						components.call(select_data.entity, interact_with{*this});
					}
					else
					{
						if (selected_entity != engine::Entity::null())
						{
							deselect(selected_entity);
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

	struct MappingData
	{
		engine::Entity callback;
		engine::Asset context;
	};

	std::array<MappingData, 5> mapping_data;

	void post_command_callback(engine::Command command, float value, void * data)
	{
		const auto & mapping_data = *static_cast<MappingData *>(data);

		gameplay::gamestate::post_command(mapping_data.callback, command, value);
	}

	void cursor_callback(engine::Command command, float value, void * data)
	{
		const auto & mapping_data = *static_cast<MappingData *>(data);

		static int x = -1;
		static int y = -1;

		switch (command)
		{
		case gameplay::command::MOUSE_CLICK:
			engine::graphics::renderer::post_select(x, y, mapping_data.callback, value == 1.f ? gameplay::command::RENDER_SELECT : gameplay::command::RENDER_DESELECT);
			break;
		case gameplay::command::MOUSE_MOVE_X:
			x = static_cast<int>(value);
			break;
		case gameplay::command::MOUSE_MOVE_Y:
			y = static_cast<int>(value);
			// first command_x is called and then command_y
			engine::graphics::renderer::post_select(x, y, mapping_data.callback, gameplay::command::RENDER_HIGHLIGHT);
			break;
		default:
			debug_unreachable("unknown command");
		}
	}
}

namespace gameplay
{
namespace gamestate
{
	void create()
	{
		std::vector<engine::Asset> states = {engine::Asset("game"), engine::Asset("debug")};
		engine::hid::ui::post_add_context(engine::Asset("default"), std::move(states));

		auto debug_camera = engine::Entity::create();
		auto game_camera = engine::Entity::create();
		core::maths::Vector3f debug_camera_pos{ 0.f, 4.f, 0.f };
		core::maths::Vector3f game_camera_pos{ 0.f, 7.f, 5.f };

		components.emplace<FreeCamera>(debug_camera, debug_camera);
		components.emplace<OverviewCamera>(game_camera, game_camera);

		engine::physics::camera::add(debug_camera, debug_camera_pos, false);
		engine::physics::camera::add(game_camera, game_camera_pos, true);

		engine::graphics::viewer::post_add_frame(engine::Asset("game"), engine::graphics::viewer::dynamic{engine::Asset("root")});

		engine::graphics::viewer::post_add_projection(engine::Asset("my-perspective-3d"), engine::graphics::viewer::perspective{core::maths::make_degree(80.), .125, 128.});
		engine::graphics::viewer::post_add_projection(engine::Asset("my-perspective-2d"), engine::graphics::viewer::orthographic{-100., 100});

		engine::graphics::viewer::post_add_camera(
				debug_camera,
				engine::graphics::viewer::camera{
					engine::Asset("my-perspective-3d"),
					engine::Asset("my-perspective-2d"),
					core::maths::Quaternionf{ 1.f, 0.f, 0.f, 0.f },
					debug_camera_pos});
		engine::graphics::viewer::post_add_camera(
				game_camera,
				engine::graphics::viewer::camera{
					engine::Asset("my-perspective-3d"),
					engine::Asset("my-perspective-2d"),
					core::maths::Quaternionf{ std::cos(make_radian(core::maths::degreef{-40.f/2.f}).get()), std::sin(make_radian(core::maths::degreef{-40.f/2.f}).get()), 0.f, 0.f },
					game_camera_pos});
		engine::graphics::viewer::post_bind(engine::Asset("game"), game_camera);

		auto flycontrol = engine::Entity::create();
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_LEFT, gameplay::command::TURN_LEFT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_RIGHT, gameplay::command::TURN_RIGHT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_DOWN, gameplay::command::TURN_DOWN);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_UP, gameplay::command::TURN_UP);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_A, gameplay::command::MOVE_LEFT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_D, gameplay::command::MOVE_RIGHT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_S, gameplay::command::MOVE_DOWN);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_W, gameplay::command::MOVE_UP);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_Q, gameplay::command::ROLL_LEFT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_E, gameplay::command::ROLL_RIGHT);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_LEFTCTRL, gameplay::command::ELEVATE_DOWN);
		engine::hid::ui::post_add_button_press(flycontrol, engine::hid::Input::Button::KEY_SPACE, gameplay::command::ELEVATE_UP);
		mapping_data[0] = MappingData{debug_camera, engine::Asset("default")};
		engine::hid::ui::post_bind(engine::Asset("default"), engine::Asset("debug"), flycontrol, post_command_callback, &mapping_data[0]);

		auto pancontrol = engine::Entity::create();
		engine::hid::ui::post_add_button_press(pancontrol, engine::hid::Input::Button::KEY_LEFT, gameplay::command::MOVE_LEFT);
		engine::hid::ui::post_add_button_press(pancontrol, engine::hid::Input::Button::KEY_RIGHT, gameplay::command::MOVE_RIGHT);
		engine::hid::ui::post_add_button_press(pancontrol, engine::hid::Input::Button::KEY_UP, gameplay::command::MOVE_UP);
		engine::hid::ui::post_add_button_press(pancontrol, engine::hid::Input::Button::KEY_DOWN, gameplay::command::MOVE_DOWN);
		engine::hid::ui::post_add_axis_tilt(pancontrol, engine::hid::Input::Axis::TILT_DPAD_X, gameplay::command::MOVE_LEFT, gameplay::command::MOVE_RIGHT);
		engine::hid::ui::post_add_axis_tilt(pancontrol, engine::hid::Input::Axis::TILT_DPAD_Y, gameplay::command::MOVE_UP, gameplay::command::MOVE_DOWN);
		engine::hid::ui::post_add_axis_tilt(pancontrol, engine::hid::Input::Axis::TILT_STICKL_X, gameplay::command::MOVE_LEFT, gameplay::command::MOVE_RIGHT);
		engine::hid::ui::post_add_axis_tilt(pancontrol, engine::hid::Input::Axis::TILT_STICKL_Y, gameplay::command::MOVE_UP, gameplay::command::MOVE_DOWN);
		mapping_data[1] = MappingData{game_camera, engine::Asset("default")};
		engine::hid::ui::post_bind(engine::Asset("default"), engine::Asset("game"), pancontrol, post_command_callback, &mapping_data[1]);

		auto debug_switch = engine::Entity::create();
		auto game_switch = engine::Entity::create();

		components.emplace<CameraActivator>(debug_switch, engine::Asset("default"), engine::Asset("debug"), engine::Asset("game"), debug_camera);
		components.emplace<CameraActivator>(game_switch, engine::Asset("default"), engine::Asset("game"), engine::Asset("game"), game_camera);

		engine::hid::ui::post_add_button_press(debug_switch, engine::hid::Input::Button::KEY_F1, gameplay::command::ACTIVATE_CAMERA);
		engine::hid::ui::post_add_button_press(game_switch, engine::hid::Input::Button::KEY_F2, gameplay::command::ACTIVATE_CAMERA);
		mapping_data[2] = MappingData{game_switch, engine::Asset("default")};
		mapping_data[3] = MappingData{debug_switch, engine::Asset("default")};
		engine::hid::ui::post_bind(engine::Asset("default"), engine::Asset("debug"), game_switch, post_command_callback, &mapping_data[2]);
		engine::hid::ui::post_bind(engine::Asset("default"), engine::Asset("game"), debug_switch, post_command_callback, &mapping_data[3]);

		auto selector = engine::Entity::create();
		components.emplace<Selector>(selector);

		auto cursor = engine::Entity::create();
		engine::hid::ui::post_add_axis_move(cursor, engine::hid::Input::Axis::MOUSE_MOVE, gameplay::command::MOUSE_MOVE_X, gameplay::command::MOUSE_MOVE_Y);
		engine::hid::ui::post_add_button_press(cursor, engine::hid::Input::Button::MOUSE_LEFT, gameplay::command::MOUSE_CLICK);
		mapping_data[4] = MappingData{selector, engine::Asset("default")};
		engine::hid::ui::post_bind(engine::Asset("default"), engine::Asset("game"), cursor, cursor_callback, &mapping_data[4]);

		// vvvv tmp vvvv
		gameplay::create_level(engine::Entity::create(), "level");

		engine::resource::reader::post_read("recipes", data_callback_recipes);
		engine::resource::reader::post_read("classes", data_callback_roles);
		engine::resource::reader::post_read("skills", data_callback_skills);
	}

	void destroy()
	{
		engine::hid::ui::post_remove_context(engine::Asset("default"));
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
		}

		// commands
		{
			std::tuple<engine::Entity, engine::Command, utility::any> command_args;
			while (queue_commands.try_pop(command_args))
			{
				engine::replay::post_add_command(frame_count, std::get<0>(command_args), std::get<1>(command_args), utility::any(std::get<2>(command_args)));

				if (debug_verify(std::get<0>(command_args) != engine::Entity::null()))
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
