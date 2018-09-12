
#include "factory.hpp"

#include <core/container/Collection.hpp>
#include <core/maths/algorithm.hpp>
#include <core/sync/CriticalSection.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>
#include <engine/resource/loader.hpp>

#include <gameplay/debug.hpp>
#include <gameplay/gamestate.hpp>

#include <utility/string.hpp>

namespace
{
	struct ResourceCharacter
	{
		struct Loaded
		{
			engine::Asset mesh;
			// engine::Asset texture;
			std::string name;

			Loaded(
				engine::Asset && mesh,
				std::string && name)
				: mesh(std::move(mesh))
				, name(std::move(name))
			{}
		};
		struct Loading
		{
			std::vector<engine::Entity> entities;
		};
		using State = utility::variant<Loading, Loaded>;

		State state;

		bool is_loading() const
		{
			return utility::holds_alternative<Loading>(state);
		}

		const Loaded & get_loaded() const
		{
			debug_assert(utility::holds_alternative<Loaded>(state));

			return utility::get<Loaded>(state);
		}

		void add(engine::Entity entity)
		{
			debug_assert(utility::holds_alternative<Loading>(state));

			utility::get<Loading>(state).entities.push_back(entity);
		}

		void load(
			engine::Asset && mesh,
			std::string && name);
	};
	void create_entity(engine::Entity entity, const ResourceCharacter::Loaded & resource);

	void ResourceCharacter::load(
		engine::Asset && mesh,
		std::string && name)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(mesh), std::move(name));

		for (engine::Entity entity : loading.entities)
		{
			create_entity(entity, loaded);
		}
	}

	struct ResourceDecoration
	{
		struct Loaded
		{
			std::vector<engine::graphics::data::CompC::asset> assets;

			Loaded(std::vector<engine::graphics::data::CompC::asset> && assets)
				: assets(std::move(assets))
			{}
		};
		struct Loading
		{
			std::vector<engine::Entity> entities;
		};
		using State = utility::variant<Loading, Loaded>;

		State state;

		bool is_loading() const
		{
			return utility::holds_alternative<Loading>(state);
		}

		const Loaded & get_loaded() const
		{
			debug_assert(utility::holds_alternative<Loaded>(state));

			return utility::get<Loaded>(state);
		}

		void add(engine::Entity entity)
		{
			debug_assert(utility::holds_alternative<Loading>(state));

			utility::get<Loading>(state).entities.push_back(entity);
		}

		void load(
			std::vector<engine::graphics::data::CompC::asset> && assets);
	};

	void create_entity(engine::Entity entity, const ResourceDecoration::Loaded & resource);

	void ResourceDecoration::load(
		std::vector<engine::graphics::data::CompC::asset> && assets)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(assets));

		for (engine::Entity entity : loading.entities)
		{
			create_entity(entity, loaded);
		}
	}

	struct ResourceTable
	{
		struct Loaded
		{
			std::vector<engine::graphics::data::CompC::asset> assets;

			core::maths::Matrix4x4f front;
			core::maths::Matrix4x4f top;

			Loaded(
				std::vector<engine::graphics::data::CompC::asset> && assets,
				core::maths::Matrix4x4f && front,
				core::maths::Matrix4x4f && top)
				: assets(std::move(assets))
				, front(std::move(front))
				, top(std::move(top))
			{}
		};
		struct Loading
		{
			std::vector<engine::Entity> entities;
		};
		using State = utility::variant<Loading, Loaded>;

		State state;

		bool is_loading() const
		{
			return utility::holds_alternative<Loading>(state);
		}

		const Loaded & get_loaded() const
		{
			debug_assert(utility::holds_alternative<Loaded>(state));

			return utility::get<Loaded>(state);
		}

		void add(engine::Entity entity)
		{
			debug_assert(utility::holds_alternative<Loading>(state));

			utility::get<Loading>(state).entities.push_back(entity);
		}

		void load(
			std::vector<engine::graphics::data::CompC::asset> && assets,
			core::maths::Matrix4x4f && front,
			core::maths::Matrix4x4f && top);
	};

	void create_entity(engine::Entity entity, const ResourceTable::Loaded & resource);

	void ResourceTable::load(
		std::vector<engine::graphics::data::CompC::asset> && assets,
		core::maths::Matrix4x4f && front,
		core::maths::Matrix4x4f && top)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(assets), std::move(front), std::move(top));

		for (engine::Entity entity : loading.entities)
		{
			create_entity(entity, loaded);
		}
	}

	core::container::UnorderedCollection
	<
		engine::Asset,
		61,
		std::array<ResourceCharacter, 10>,
		std::array<ResourceDecoration, 10>,
		std::array<ResourceTable, 10>
	> resources;

	core::sync::CriticalSection resources_cs;
}

namespace
{
	struct EntityBench
	{
		struct Created
		{
			engine::Entity entity;

			~Created()
			{
				if (entity != engine::Entity::null())
				{
					// gameplay::gamestate::post_remove(entity);
					engine::graphics::renderer::post_remove(entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceTable::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				engine::graphics::renderer::post_add_component(
					entity,
					engine::graphics::data::CompC{
						modelview,
							core::maths::Vector3f{ 1.f, 1.f, 1.f },
							resource.assets });
				engine::graphics::renderer::post_make_selectable(entity);

				gameplay::gamestate::post_add_workstation(
					entity,
					gameplay::gamestate::WorkstationType::BENCH,
					modelview * resource.front,
					modelview * resource.top);
			}
			Created(Created && x)
				: entity(x.entity)
			{
				x.entity = engine::Entity::null();
			}
			Created & operator = (Created && x)
			{
				using std::swap;
				swap(entity, x.entity);
				return *this;
			}
		};
		struct Creating
		{
			core::maths::Matrix4x4f modelview;
		};
		using State = utility::variant<Creating, Created>;

		State state;

		EntityBench(const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Creating>, modelview)
		{}
		EntityBench(engine::Entity entity, const ResourceTable::Loaded & resource, const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Created>, entity, resource, modelview)
		{}

		void create(engine::Entity entity, const ResourceTable::Loaded & resource)
		{
			debug_assert(utility::holds_alternative<Creating>(state));

			Creating creating = utility::get<Creating>(std::move(state));
			state.emplace<Created>(entity, resource, creating.modelview);
		}
	};
	struct EntityBoard
	{
		struct Created
		{
			engine::Entity entity;

			~Created()
			{
				if (entity != engine::Entity::null())
				{
					engine::graphics::renderer::post_remove(entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceDecoration::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				engine::graphics::renderer::post_add_component(
					entity,
					engine::graphics::data::CompC{
						modelview,
							core::maths::Vector3f{ 1.f, 1.f, 1.f },
							resource.assets });
			}
			Created(Created && x)
				: entity(x.entity)
			{
				x.entity = engine::Entity::null();
			}
			Created & operator = (Created && x)
			{
				using std::swap;
				swap(entity, x.entity);
				return *this;
			}
		};
		struct Creating
		{
			core::maths::Matrix4x4f modelview;
		};
		using State = utility::variant<Creating, Created>;

		State state;

		EntityBoard(const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Creating>, modelview)
		{}
		EntityBoard(engine::Entity entity, const ResourceDecoration::Loaded & resource, const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Created>, entity, resource, modelview)
		{}

		void create(engine::Entity entity, const ResourceDecoration::Loaded & resource)
		{
			debug_assert(utility::holds_alternative<Creating>(state));

			Creating creating = utility::get<Creating>(std::move(state));
			state.emplace<Created>(entity, resource, creating.modelview);
		}
	};
	struct EntityOven
	{
		struct Created
		{
			engine::Entity entity;

			~Created()
			{
				if (entity != engine::Entity::null())
				{
					// gameplay::gamestate::post_remove(entity);
					engine::graphics::renderer::post_remove(entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceTable::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				engine::graphics::renderer::post_add_component(
					entity,
					engine::graphics::data::CompC{
						modelview,
							core::maths::Vector3f{ 1.f, 1.f, 1.f },
							resource.assets });
				engine::graphics::renderer::post_make_selectable(entity);

				gameplay::gamestate::post_add_workstation(
					entity,
					gameplay::gamestate::WorkstationType::OVEN,
					modelview * resource.front,
					modelview * resource.top);
			}
			Created(Created && x)
				: entity(x.entity)
			{
				x.entity = engine::Entity::null();
			}
			Created & operator = (Created && x)
			{
				using std::swap;
				swap(entity, x.entity);
				return *this;
			}
		};
		struct Creating
		{
			core::maths::Matrix4x4f modelview;
		};
		using State = utility::variant<Creating, Created>;

		State state;

		EntityOven(const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Creating>, modelview)
		{}
		EntityOven(engine::Entity entity, const ResourceTable::Loaded & resource, const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Created>, entity, resource, modelview)
		{}

		void create(engine::Entity entity, const ResourceTable::Loaded & resource)
		{
			debug_assert(utility::holds_alternative<Creating>(state));

			Creating creating = utility::get<Creating>(std::move(state));
			state.emplace<Created>(entity, resource, creating.modelview);
		}
	};
	struct EntityWorker
	{
		struct Created
		{
			engine::Entity entity;

			~Created()
			{
				if (entity != engine::Entity::null())
				{
					// gameplay::gamestate::post_remove(entity);
					// engine::animation::post_remove(entity);
					engine::physics::post_remove(entity);
					engine::graphics::renderer::post_remove(entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceCharacter::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				engine::graphics::renderer::post_add_character(
					entity,
					engine::graphics::data::CompT{
						modelview,
							core::maths::Vector3f { 1.f, 1.f, 1.f },
							resource.mesh,
								engine::Asset{ "dude" }});
				engine::graphics::renderer::post_make_selectable(entity);

				core::maths::Vector3f translation;
				core::maths::Quaternionf rotation;
				core::maths::Vector3f scale;
				decompose(modelview, translation, rotation, scale);
				engine::physics::post_add_object(entity, engine::transform_t{translation, rotation});
				engine::animation::add(entity, engine::animation::armature{utility::to_string("res/", resource.name, ".arm")});
				engine::animation::update(entity, engine::animation::action{"turn-left", true});

				gameplay::gamestate::post_add_worker(entity);
			}
			Created(Created && x)
				: entity(x.entity)
			{
				x.entity = engine::Entity::null();
			}
			Created & operator = (Created && x)
			{
				using std::swap;
				swap(entity, x.entity);
				return *this;
			}
		};
		struct Creating
		{
			core::maths::Matrix4x4f modelview;
		};
		using State = utility::variant<Creating, Created>;

		State state;

		EntityWorker(const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Creating>, modelview)
		{}
		EntityWorker(engine::Entity entity, const ResourceCharacter::Loaded & resource, const core::maths::Matrix4x4f & modelview)
			: state(utility::in_place_type<Created>, entity, resource, modelview)
		{}

		void create(engine::Entity entity, const ResourceCharacter::Loaded & resource)
		{
			debug_assert(utility::holds_alternative<Creating>(state));

			Creating creating = utility::get<Creating>(std::move(state));
			state.emplace<Created>(entity, resource, creating.modelview);
		}
	};

	core::container::Collection
	<
		engine::Entity,
		801,
		std::array<EntityBench, 100>,
		std::array<EntityBoard, 100>,
		std::array<EntityOven, 100>,
		std::array<EntityWorker, 100>
	> entities;

	void create_entity(engine::Entity entity, const ResourceCharacter::Loaded & resource)
	{
		struct CreateEntity
		{
			const ResourceCharacter::Loaded & resource;

			void operator () (engine::Entity entity, EntityWorker & x)
			{
				x.create(entity, resource);
			}
			void operator () (EntityBench & x) { debug_unreachable(); }
			void operator () (EntityBoard & x) { debug_unreachable(); }
			void operator () (EntityOven & x) { debug_unreachable(); }
		};
		entities.call(entity, CreateEntity{resource});
	}
	void create_entity(engine::Entity entity, const ResourceDecoration::Loaded & resource)
	{
		struct CreateEntity
		{
			const ResourceDecoration::Loaded & resource;

			void operator () (engine::Entity entity, EntityBoard & x)
			{
				x.create(entity, resource);
			}
			void operator () (EntityBench & x) { debug_unreachable(); }
			void operator () (EntityOven & x) { debug_unreachable(); }
			void operator () (EntityWorker & x) { debug_unreachable(); }
		};
		entities.call(entity, CreateEntity{resource});
	}
	void create_entity(engine::Entity entity, const ResourceTable::Loaded & resource)
	{
		struct CreateEntity
		{
			const ResourceTable::Loaded & resource;

			void operator () (engine::Entity entity, EntityBench & x)
			{
				x.create(entity, resource);
			}
			void operator () (engine::Entity entity, EntityOven & x)
			{
				x.create(entity, resource);
			}
			void operator () (EntityBoard & x) { debug_unreachable(); }
			void operator () (EntityWorker & x) { debug_unreachable(); }
		};
		entities.call(entity, CreateEntity{resource});
	}
}

namespace
{
	void load_level_callback(std::string name, const engine::resource::loader::Level & data)
	{
		for (const auto & mesh : data.meshes)
		{
			const engine::Asset asset = mesh.name;

			std::vector<engine::graphics::data::CompC::asset> assets;

			assets.emplace_back(engine::graphics::data::CompC::asset{asset, mesh.color});

			const auto entity = engine::Entity::create();
			engine::graphics::renderer::post_add_component(
				entity,
				engine::graphics::data::CompC{
					mesh.matrix,
						Vector3f{1.f, 1.f, 1.f},
						assets});
			engine::graphics::renderer::post_make_obstruction(entity);
		}
		for (auto & placeholder : data.placeholders)
		{
			const auto modelview =
				make_translation_matrix(placeholder.translation) *
				make_matrix(placeholder.rotation) *
				make_scale_matrix(placeholder.scale);
			gameplay::create_placeholder(placeholder.name, engine::Entity::create(), modelview);
		}
	}

	auto extract(
		const engine::resource::loader::asset_template_t & assetTemplate,
		const std::string & name)
	{
		const auto & components = assetTemplate.part(name);
		std::vector<engine::graphics::data::CompC::asset> assets;
		for (const auto & c : components)
		{
			assets.emplace_back(
				engine::graphics::data::CompC::asset{ c.mesh, c.color });
		}
		return assets;
	}

	void load_placeholder_callback(std::string name, const engine::resource::loader::Placeholder & data)
	{
		struct LoadPlaceholder
		{
			std::string && name;

			void operator () (int)
			{
				engine::Asset asset = name;

				std::lock_guard<core::sync::CriticalSection> load(resources_cs);
				debug_assert(resources.contains(asset));
				debug_assert(resources.contains<ResourceCharacter>(asset));
				resources.get<ResourceCharacter>(asset).load(std::move(asset), std::move(name));
			}
			void operator () (const engine::resource::loader::asset_template_t & x)
			{
				const engine::Asset asset = name;
				debug_printline(gameplay::gameplay_channel, "KJHS: ", name);

				std::lock_guard<core::sync::CriticalSection> load(resources_cs);
				debug_assert(resources.contains(asset));
				switch (asset)
				{
				case engine::Asset{ "bench" }:
					debug_assert(resources.contains<ResourceTable>(asset));
					resources.get<ResourceTable>(asset).load(extract(x, "bench"), x.location("front").transform.matrix(), x.location("top").transform.matrix());
					break;
				case engine::Asset{ "board" }:
					debug_assert(resources.contains<ResourceDecoration>(asset));
					resources.get<ResourceDecoration>(asset).load(extract(x, "all"));
					break;
				case engine::Asset{ "oven" }:
					debug_assert(resources.contains<ResourceTable>(asset));
					resources.get<ResourceTable>(asset).load(extract(x, "oven"), x.location("front").transform.matrix(), x.location("top").transform.matrix());
					break;
				case engine::Asset{ "dude1" }:
				case engine::Asset{ "dude2" }:
					break;
				// {
				// 	const auto & assets = extract(assetTemplate, "dude");
				// 	break;
				// }
				default:
					debug_fail();
					break;
				}
			}
		};
		visit(LoadPlaceholder{std::move(name)}, std::move(data.data));
	}
}

namespace
{
}

namespace gameplay
{
	void create_bench(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "bench";
		constexpr const auto asset = engine::Asset(name);

		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		if (resources.contains(asset))
		{
			debug_assert(resources.contains<ResourceTable>(asset));

			auto & resource = resources.get<ResourceTable>(asset);
			if (resource.is_loading())
			{
				entities.emplace<EntityBench>(entity, modelview);
				resource.add(entity);
			}
			else
			{
				entities.emplace<EntityBench>(entity, entity, resource.get_loaded(), modelview);
				// resource.add(entity);
			}
		}
		else
		{
			entities.emplace<EntityBench>(entity, modelview);
			auto & resource = resources.emplace<ResourceTable>(asset);
			resource.add(entity);
			engine::resource::loader::post_load_placeholder(name, load_placeholder_callback);
		}
	}

	void create_board(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "board";
		constexpr const auto asset = engine::Asset(name);

		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		if (resources.contains(asset))
		{
			debug_assert(resources.contains<ResourceDecoration>(asset));

			auto & resource = resources.get<ResourceDecoration>(asset);
			if (resource.is_loading())
			{
				entities.emplace<EntityBoard>(entity, modelview);
				resource.add(entity);
			}
			else
			{
				entities.emplace<EntityBoard>(entity, entity, resource.get_loaded(), modelview);
				// resource.add(entity);
			}
		}
		else
		{
			entities.emplace<EntityBoard>(entity, modelview);
			auto & resource = resources.emplace<ResourceDecoration>(asset);
			resource.add(entity);
			engine::resource::loader::post_load_placeholder(name, load_placeholder_callback);
		}
	}

	void create_oven(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "oven";
		constexpr const auto asset = engine::Asset(name);

		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		if (resources.contains(asset))
		{
			debug_assert(resources.contains<ResourceTable>(asset));

			auto & resource = resources.get<ResourceTable>(asset);
			if (resource.is_loading())
			{
				entities.emplace<EntityBench>(entity, modelview);
				resource.add(entity);
			}
			else
			{
				entities.emplace<EntityBench>(entity, entity, resource.get_loaded(), modelview);
				// resource.add(entity);
			}
		}
		else
		{
			entities.emplace<EntityBench>(entity, modelview);
			auto & resource = resources.emplace<ResourceTable>(asset);
			resource.add(entity);
			engine::resource::loader::post_load_placeholder(name, load_placeholder_callback);
		}
	}

	// void create_static(engine::Entity entity)
	// {
	// 	entities.emplace<EntityStatic>(entity);

	// 	engine::graphics::renderer::post_add_component(
	// 		entity,
	// 		engine::graphics::data::CompC{
	// 			data.matrix,
	// 				Vector3f{1.f, 1.f, 1.f},
	// 				assets});
	// 	engine::graphics::renderer::post_make_obstruction(entity);
	// }

	void create_worker(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "dude2";
		constexpr const auto asset = engine::Asset(name);

		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		if (resources.contains(asset))
		{
			debug_assert(resources.contains<ResourceCharacter>(asset));

			auto & resource = resources.get<ResourceCharacter>(asset);
			if (resource.is_loading())
			{
				entities.emplace<EntityWorker>(entity, modelview);
				resource.add(entity);
			}
			else
			{
				entities.emplace<EntityWorker>(entity, entity, resource.get_loaded(), modelview);
				// resource.add(entity);
			}
		}
		else
		{
			entities.emplace<EntityWorker>(entity, modelview);
			auto & resource = resources.emplace<ResourceCharacter>(asset);
			resource.add(entity);
			engine::resource::loader::post_load_placeholder(name, load_placeholder_callback);
		}
	}

	// void create_worker_deprecated(engine::Entity entity)
	// {
	// 	entities.emplace<EntityWorkerDeprecated>(entity);

	// 	engine::graphics::renderer::post_add_component(
	// 		id,
	// 		engine::graphics::data::CompC{
	// 			matrix,
	// 				Vector3f{ 1.f, 1.f, 1.f },
	// 				assets });
	// 	engine::graphics::renderer::post_make_obstruction(id);

	// 	gameplay::gamestate::post_add_worker(id);
	// }

	void create_level(engine::Entity entity, const std::string & name)
	{
		engine::resource::loader::post_load_level(name, load_level_callback);

		// tmp
		resources.emplace<ResourceDecoration>("board");
		engine::resource::loader::post_load_placeholder("board", load_placeholder_callback);
	}

	void create_placeholder(engine::Asset asset, engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		switch (asset)
		{
		case engine::Asset{"bench"}:
			return create_bench(entity, modelview);
		case engine::Asset{"board"}:
			return create_board(entity, modelview);
		case engine::Asset{"oven"}:
			return create_oven(entity, modelview);
		case engine::Asset{"dude1"}:
			break;
		case engine::Asset{"dude2"}:
			return create_worker(entity, modelview);
		default:
			debug_fail();
		}
	}

	void destroy(engine::Entity entity)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		entities.remove(entity);
	}
}
