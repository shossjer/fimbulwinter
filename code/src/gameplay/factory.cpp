
#include "factory.hpp"

#include "core/color.hpp"
#include "core/container/Collection.hpp"
#include "core/maths/algorithm.hpp"
#include "core/sync/CriticalSection.hpp"

#include "engine/animation/mixer.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/physics/physics.hpp"
#include "engine/resource/reader.hpp"

#include "gameplay/debug.hpp"
#include "gameplay/gamestate.hpp"

#include "utility/ranges.hpp"
#include "utility/string.hpp"

#include <unordered_map>

namespace
{
	engine::animation::mixer * mixer = nullptr;
	engine::graphics::renderer * renderer = nullptr;
	engine::physics::simulation * simulation = nullptr;
	engine::resource::reader * reader = nullptr;
	gameplay::gamestate * gamestate = nullptr;

	struct LevelData
	{
		struct Mesh
		{
			std::string name;
			core::maths::Matrix4x4f matrix;
			float color[3];
			core::container::Buffer vertices;
			core::container::Buffer triangles;
			core::container::Buffer normals;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("name"), &Mesh::name),
					std::make_pair(utility::string_view("matrix"), &Mesh::matrix),
					std::make_pair(utility::string_view("color"), &Mesh::color),
					std::make_pair(utility::string_view("vertices"), &Mesh::vertices),
					std::make_pair(utility::string_view("triangles"), &Mesh::triangles),
					std::make_pair(utility::string_view("normals"), &Mesh::normals)
					);
			}
		};
		struct Placeholder
		{
			std::string name;
			core::maths::Vector3f translation;
			core::maths::Quaternionf rotation;
			core::maths::Vector3f scale;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("name"), &Placeholder::name),
					std::make_pair(utility::string_view("translation"), &Placeholder::translation),
					std::make_pair(utility::string_view("rotation"), &Placeholder::rotation),
					std::make_pair(utility::string_view("scale"), &Placeholder::scale)
					);
			}
		};

		std::vector<Mesh> meshes;
		std::vector<Placeholder> placeholders;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("meshes"), &LevelData::meshes),
				std::make_pair(utility::string_view("placeholders"), &LevelData::placeholders)
				);
		}
	};

	struct ModelData
	{
		struct Part
		{
			struct Render
			{
				struct Shape
				{
					enum class Type
					{
						MESH
					} type;

					core::rgb_t<double> color;
					core::container::Buffer vertices;
					core::container::Buffer indices;
					core::container::Buffer normals;

					static constexpr auto serialization()
					{
						return utility::make_lookup_table(
							// std::make_pair(utility::string_view("type"), &Shape::type),
							std::make_pair(utility::string_view("color"), &Shape::color),
							std::make_pair(utility::string_view("indices"), &Shape::indices),
							std::make_pair(utility::string_view("normals"), &Shape::normals),
							std::make_pair(utility::string_view("vertices"), &Shape::vertices)
							);
					}
				};

				std::vector<Shape> shapes;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_view("shapes"), &Render::shapes)
						);
				}
			};

			struct Physic
			{
				struct Shape
				{
					engine::transform_t transform;

					enum class Type
					{
						BOX,
						MESH
					} type;

					// std::vector<core::maths::Vector3f> points;
					// float w, h, d;
					float scale[3];

					static constexpr auto serialization()
					{
						return utility::make_lookup_table(
							std::make_pair(utility::string_view("transform"), &Shape::transform),
							// std::make_pair(utility::string_view("type"), &Shape::type),
							std::make_pair(utility::string_view("scale"), &Shape::scale)
							);
					}
				};

				std::vector<Shape> shapes;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_view("shapes"), &Physic::shapes)
						);
				}
			};

			std::string name;
			Render render;
			Physic physic;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("name"), &Part::name),
					std::make_pair(utility::string_view("render"), &Part::render),
					std::make_pair(utility::string_view("physic"), &Part::physic)
					);
			}
		};

		std::unordered_map<std::string, Part> parts;

		struct Joint
		{
			std::string name;

			struct Dummy
			{
				engine::transform_t transform;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_view("transform"), &Dummy::transform)
						);
				}
			} dummy;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("name"), &Joint::name),
					std::make_pair(utility::string_view("joint"), &Joint::dummy)
					);
			}
		};

		std::vector<Joint> joints;

		struct Location
		{
			std::string name;
			engine::transform_t transform;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("name"), &Location::name),
					std::make_pair(utility::string_view("transform"), &Location::transform)
					);
			}
		};

		std::vector<Location> locations;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("parts"), &ModelData::parts),
				std::make_pair(utility::string_view("relations"), &ModelData::joints),
				std::make_pair(utility::string_view("locations"), &ModelData::locations)
				);
		}

		const Part & part(const std::string & name) const
		{
			auto maybe_part = parts.find(name);
			debug_assert(maybe_part != parts.end());
			return maybe_part->second;
		}

		const Joint & joint(const std::string & name) const
		{
			return get(joints, name);
		}

		const Location & location(const std::string & name) const
		{
			return get(locations, name);
		}
	private:
		template<typename T>
		const T & get(const std::vector<T> & items, const std::string & name) const
		{
			for (const auto & i : items)
			{
				if (i.name == name) return i;
			}

			debug_unreachable();
		}
	};
}

namespace
{
	void create_placeholder_lockfree(engine::Asset asset, engine::Entity entity, const core::maths::Matrix4x4f & modelview);

	struct ResourceCharacter
	{
		struct Loaded
		{
			engine::Asset armature;
			engine::Asset mesh;

			~Loaded()
			{
				// engine::animation::post_unregister(armature);
				// engine::graphics::renderer::post_unregister(mesh);
			}
			Loaded(engine::Asset armature, engine::Asset mesh)
				: armature(armature)
				, mesh(mesh)
			{}
		};
		struct Loading
		{
			std::vector<engine::Entity> entities;

			engine::Asset armature = engine::Asset::null();
			engine::Asset mesh = engine::Asset::null();

			Loading()
			{}

			bool set(engine::Asset && asset, std::string && name, engine::model::mesh_t && data)
			{
				debug_assert(mesh == engine::Asset::null());
				mesh = asset;

				debug_printline("registering character \"", name, "\"");
				post_register_character(*::renderer, asset, std::move(data));

				return armature != engine::Asset::null();
			}
			bool set(engine::Asset && asset, std::string && name, engine::animation::Armature && data)
			{
				debug_assert(armature == engine::Asset::null());
				armature = asset;

				post_register_armature(*::mixer, asset, std::move(data));

				return mesh != engine::Asset::null();
			}
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

		void load(Loading loading);
		void load(engine::Asset && asset, std::string && name, engine::model::mesh_t && data)
		{
			debug_assert(utility::holds_alternative<Loading>(state));
			auto & loading = utility::get<Loading>(state);

			if (loading.set(std::move(asset), std::move(name), std::move(data)))
				load(std::move(loading));
		}
		void load(engine::Asset && asset, std::string && name, engine::animation::Armature && data)
		{
			debug_assert(utility::holds_alternative<Loading>(state));
			auto & loading = utility::get<Loading>(state);

			if (loading.set(std::move(asset), std::move(name), std::move(data)))
				load(std::move(loading));
		}
	};

	void create_entity(engine::Entity entity, const ResourceCharacter::Loaded & resource);

	void ResourceCharacter::load(Loading loading)
	{
		const auto& loaded = state.emplace<Loaded>(loading.armature, loading.mesh);

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

			~Loaded()
			{
				// for (const auto & asset : assets)
				// {
				// 	engine::graphics::renderer::post_unregister(asset.mesh);
				// }
			}
			Loaded(engine::Asset && asset, std::string && name, ModelData && data)
			{
				const auto & part = data.part("all");

				for (int i : ranges::index_sequence_for(part.render.shapes))
				{
					const auto & mesh = part.render.shapes[i];
					const engine::Asset meshId(name + part.name + std::to_string(i));

					post_register_mesh(
						*::renderer,
						meshId,
						engine::graphics::data::Mesh{
							mesh.vertices,
							mesh.indices,
							mesh.normals,
							core::container::Buffer{} });

					const uint32_t r = static_cast<uint8_t>(mesh.color.red() * 255);
					const uint32_t g = static_cast<uint8_t>(mesh.color.green() * 255);
					const uint32_t b = static_cast<uint8_t>(mesh.color.blue() * 255);
					const auto color = r + (g << 8) + (b << 16) + (0xff << 24);

					assets.push_back({meshId, color});
				}
			}
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

		void load(engine::Asset && asset, std::string && name, ModelData && data);
	};

	void create_entity(engine::Entity entity, const ResourceDecoration::Loaded & resource);

	void ResourceDecoration::load(engine::Asset && asset, std::string && name, ModelData && data)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(asset), std::move(name), std::move(data));

		for (engine::Entity entity : loading.entities)
		{
			create_entity(entity, loaded);
		}
	}

	struct ResourceLevel
	{
		struct Loaded
		{
			struct Mesh
			{
				std::string name;
				core::maths::Matrix4x4f matrix;
				uint32_t color;
			};

			struct Placeholder
			{
				std::string name;
				engine::Entity entity;
			};

			std::vector<Mesh> meshes;
			std::vector<Placeholder> placeholders;

			~Loaded()
			{
				// for (const auto & mesh : meshes)
				// {
				// 	const engine::Asset asset = mesh.name;

				// 	engine::graphics::renderer::post_unregister(asset);
				// }
			}
			Loaded(engine::Asset && asset, std::string && name, LevelData && data)
			{
				// tmp
				set(
					*::simulation,
					engine::physics::camera::Bounds{
						core::maths::Vector3f{-6.f, 0.f, -4.f},
						core::maths::Vector3f{6.f, 10.f, 12.f}});

				for (const auto & mesh : data.meshes)
				{
					const engine::Asset mesh_asset(mesh.name);

					post_register_mesh(
						*renderer,
						mesh_asset,
						engine::graphics::data::Mesh{
							mesh.vertices,
							mesh.triangles,
							mesh.normals,
							core::container::Buffer{}});

					const uint32_t r = static_cast<uint8_t>(mesh.color[0] * 255);
					const uint32_t g = static_cast<uint8_t>(mesh.color[1] * 255);
					const uint32_t b = static_cast<uint8_t>(mesh.color[2] * 255);
					const auto color = r + (g << 8) + (b << 16) + (0xff << 24);

					meshes.push_back({mesh.name, mesh.matrix, color});
				}

				for (const auto & placeholder : data.placeholders)
				{
					const auto entity = engine::Entity::create();
					const auto modelview =
						make_translation_matrix(placeholder.translation) *
						make_matrix(placeholder.rotation) *
						make_scale_matrix(placeholder.scale);

					create_placeholder_lockfree(engine::Asset(placeholder.name), entity, modelview);

					placeholders.push_back({placeholder.name, entity});
				}
			}
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

		void load(engine::Asset && asset, std::string && name, LevelData && data);
	};

	void create_entity(engine::Entity entity, const ResourceLevel::Loaded & resource);

	void ResourceLevel::load(engine::Asset && asset, std::string && name, LevelData && data)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(asset), std::move(name), std::move(data));

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

			~Loaded()
			{
				// for (const auto & asset : assets)
				// {
				// 	engine::graphics::renderer::post_unregister(asset.mesh);
				// }
			}
			Loaded(engine::Asset && asset, std::string && name, ModelData && data)
			{
				const auto & part = data.part(name);

				for (int i : ranges::index_sequence_for(part.render.shapes))
				{
					const auto & mesh = part.render.shapes[i];
					const engine::Asset meshId(name + part.name + std::to_string(i));

					post_register_mesh(
						*renderer,
						meshId,
						engine::graphics::data::Mesh{
							mesh.vertices,
							mesh.indices,
							mesh.normals,
							core::container::Buffer{} });

					const uint32_t r = static_cast<uint8_t>(mesh.color.red() * 255);
					const uint32_t g = static_cast<uint8_t>(mesh.color.green() * 255);
					const uint32_t b = static_cast<uint8_t>(mesh.color.blue() * 255);
					const auto color = r + (g << 8) + (b << 16) + (0xff << 24);

					assets.push_back({meshId, color});
				}

				front = data.location("front").transform.matrix();
				top = data.location("top").transform.matrix();
			}
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

		void load(engine::Asset && asset, std::string && name, ModelData && data);
	};

	void create_entity(engine::Entity entity, const ResourceTable::Loaded & resource);

	void ResourceTable::load(engine::Asset && asset, std::string && name, ModelData && data)
	{
		debug_assert(utility::holds_alternative<Loading>(state));

		Loading loading = utility::get<Loading>(std::move(state));
		const auto& loaded = state.emplace<Loaded>(std::move(asset), std::move(name), std::move(data));

		for (engine::Entity entity : loading.entities)
		{
			create_entity(entity, loaded);
		}
	}

	core::container::UnorderedCollection
	<
		engine::Asset,
		101,
		std::array<ResourceCharacter, 10>,
		std::array<ResourceDecoration, 10>,
		std::array<ResourceLevel, 10>,
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
					post_remove(*::renderer, entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceTable::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				post_add_component(
					*::renderer,
					entity,
					engine::graphics::data::CompC{
						modelview,
						core::maths::Vector3f{ 1.f, 1.f, 1.f },
						resource.assets });
				post_make_selectable(*::renderer, entity);

				post_add_workstation(
					*::gamestate,
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
					post_remove(*::renderer, entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceDecoration::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				post_add_component(
					*::renderer,
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

	struct EntityLevel
	{
		struct Created
		{
			engine::Entity entity;

			std::vector<engine::Entity> meshes;

			~Created()
			{
				if (entity != engine::Entity::null())
				{
					for (engine::Entity mesh_entity : meshes)
					{
						post_remove(*::renderer, mesh_entity);
					}
				}
			}
			Created(engine::Entity entity,
			        const ResourceLevel::Loaded & resource)
				: entity(entity)
			{
				for (const auto & mesh : resource.meshes)
				{
					const engine::Asset asset(mesh.name);

					std::vector<engine::graphics::data::CompC::asset> assets;
					assets.push_back({asset, mesh.color});

					const auto mesh_entity = engine::Entity::create();

					post_add_component(
						*::renderer,
						mesh_entity,
						engine::graphics::data::CompC{
							mesh.matrix,
							core::maths::Vector3f{1.f, 1.f, 1.f},
							assets});
					post_make_obstruction(*::renderer, mesh_entity);

					meshes.push_back(mesh_entity);
				}
			}
			Created(Created && x)
				: entity(x.entity)
				, meshes(std::move(x.meshes))
			{
				x.entity = engine::Entity::null();
			}
			Created & operator = (Created && x)
			{
				using std::swap;
				swap(entity, x.entity);
				swap(meshes, x.meshes);
				return *this;
			}
		};
		struct Creating
		{
		};
		using State = utility::variant<Creating, Created>;

		State state;

		EntityLevel()
			: state(utility::in_place_type<Creating>)
		{}
		EntityLevel(engine::Entity entity,
		            const ResourceLevel::Loaded & resource)
			: state(utility::in_place_type<Created>, entity, resource)
		{}

		void create(engine::Entity entity,
		            const ResourceLevel::Loaded & resource)
		{
			debug_assert(utility::holds_alternative<Creating>(state));

			Creating creating = utility::get<Creating>(std::move(state));
			state.emplace<Created>(entity, resource);
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
					post_remove(*::renderer, entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceTable::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				post_add_component(
					*::renderer,
					entity,
					engine::graphics::data::CompC{
						modelview,
						core::maths::Vector3f{ 1.f, 1.f, 1.f },
						resource.assets });
				post_make_selectable(*::renderer, entity);

				post_add_workstation(
					*::gamestate,
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
					post_remove(*::simulation, entity);
					post_remove(*::renderer, entity);
				}
			}
			Created(
				engine::Entity entity,
				const ResourceCharacter::Loaded & resource,
				const core::maths::Matrix4x4f & modelview)
				: entity(entity)
			{
				post_add_character(
					*::renderer,
					entity,
					engine::graphics::data::CompT{
						modelview,
						core::maths::Vector3f { 1.f, 1.f, 1.f },
						resource.mesh,
						engine::Asset{ "dude" }});
				post_make_selectable(*::renderer, entity);

				core::maths::Vector3f translation;
				core::maths::Quaternionf rotation;
				core::maths::Vector3f scale;
				decompose(modelview, translation, rotation, scale);
				post_add_object(*::simulation, entity, engine::transform_t{translation, rotation});
				post_add_character(*::mixer, entity, engine::animation::character{resource.armature});
				post_update_action(*::mixer, entity, engine::animation::action{"turn-left", true});

				post_add_worker(*::gamestate, entity);
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
		utility::heap_storage<EntityBench>,
		utility::heap_storage<EntityBoard>,
		utility::heap_storage<EntityLevel>,
		utility::heap_storage<EntityOven>,
		utility::heap_storage<EntityWorker>
	>
	entities;

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
			void operator () (EntityLevel & x) { debug_unreachable(); }
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
			void operator () (EntityLevel & x) { debug_unreachable(); }
			void operator () (EntityOven & x) { debug_unreachable(); }
			void operator () (EntityWorker & x) { debug_unreachable(); }
		};
		entities.call(entity, CreateEntity{resource});
	}

	void create_entity(engine::Entity entity, const ResourceLevel::Loaded & resource)
	{
		struct CreateEntity
		{
			const ResourceLevel::Loaded & resource;

			void operator () (engine::Entity entity, EntityLevel & x)
			{
				x.create(entity, resource);
			}
			void operator () (EntityBench & x) { debug_unreachable(); }
			void operator () (EntityBoard & x) { debug_unreachable(); }
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
			void operator () (EntityLevel & x) { debug_unreachable(); }
			void operator () (EntityWorker & x) { debug_unreachable(); }
		};
		entities.call(entity, CreateEntity{resource});
	}
}

namespace
{
	struct TryReadArmature
	{
		std::string && name;

		void operator () (core::ArmatureStructurer && x)
		{
			engine::animation::Armature data;
			x.read(data);

			auto asset = engine::Asset(name);

			std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
			debug_assert(resources.contains<ResourceCharacter>(asset));

			auto & resource = resources.get<ResourceCharacter>(asset);
			resource.load(std::move(asset), std::move(name), std::move(data));
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_fail("not possible to serialize");
		}
	};

	void read_armature_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		visit(TryReadArmature{std::move(name)}, std::move(structurer));
	}

	struct TryReadCharacter
	{
		std::string && name;

		void operator () (core::PlaceholderStructurer && x)
		{
			engine::model::mesh_t data;
			x.read(data);

			auto asset = engine::Asset(name);

			std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
			debug_assert(resources.contains<ResourceCharacter>(asset));

			auto & resource = resources.get<ResourceCharacter>(asset);
			resource.load(std::move(asset), std::move(name), std::move(data));
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_fail("not possible to serialize");
		}
	};

	void read_character_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		visit(TryReadCharacter{std::move(name)}, std::move(structurer));
	}

	struct TryReadDecoration
	{
		std::string && name;

		void operator () (core::JsonStructurer && x)
		{
			ModelData data;
			x.read(data);

			auto asset = engine::Asset(name);

			std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
			debug_assert(resources.contains<ResourceDecoration>(asset));

			auto & resource = resources.get<ResourceDecoration>(asset);
			resource.load(std::move(asset), std::move(name), std::move(data));
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_fail("not possible to serialize");
		}
	};

	void read_decoration_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		visit(TryReadDecoration{std::move(name)}, std::move(structurer));
	}

	struct TryReadLevel
	{
		LevelData & data;

		void operator () (core::LevelStructurer && x)
		{
			x.read(data);
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_fail("not possible to serialize");
		}
	};

	void read_level_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		LevelData data;
		visit(TryReadLevel{data}, std::move(structurer));

		auto asset = engine::Asset(name);

		std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
		debug_assert(resources.contains<ResourceLevel>(asset));

		auto & resource = resources.get<ResourceLevel>(asset);
		resource.load(std::move(asset), std::move(name), std::move(data));
	}

	struct TryReadTable
	{
		std::string && name;

		void operator () (core::JsonStructurer && x)
		{
			ModelData data;
			x.read(data);

			auto asset = engine::Asset(name);

			std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
			debug_assert(resources.contains<ResourceTable>(asset));

			auto & resource = resources.get<ResourceTable>(asset);
			resource.load(std::move(asset), std::move(name), std::move(data));
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_fail("not possible to serialize");
		}
	};

	void read_table_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		visit(TryReadTable{std::move(name)}, std::move(structurer));
	}

	template <typename Resource, typename Entity, typename ...Ps>
	void create_entity_lockfree(const std::string & name, engine::Asset asset, void (* read_callback)(std::string name, engine::resource::reader::Structurer && structurer), engine::Entity entity, Ps && ...ps)
	{
		if (resources.contains(asset))
		{
			debug_assert(resources.contains<Resource>(asset));

			auto & resource = resources.get<Resource>(asset);
			if (resource.is_loading())
			{
				debug_verify(entities.try_emplace<Entity>(entity, std::forward<Ps>(ps)...));
				resource.add(entity);
			}
			else
			{
				debug_verify(entities.try_emplace<Entity>(entity, entity, resource.get_loaded(), std::forward<Ps>(ps)...));
				// resource.add(entity);
			}
		}
		else
		{
			debug_verify(entities.try_emplace<Entity>(entity, std::forward<Ps>(ps)...));
			auto & resource = resources.emplace<Resource>(asset);
			resource.add(entity);
			reader->post_read(name, read_callback);
		}
	}

	void create_level_lockfree(engine::Entity entity, const std::string & name)
	{
		create_entity_lockfree<ResourceLevel, EntityLevel>(name, engine::Asset(name), read_level_callback, entity);
	}

	void create_bench_lockfree(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "bench";
		create_entity_lockfree<ResourceTable, EntityBench>(name, engine::Asset(name), read_table_callback, entity, modelview);
	}

	void create_board_lockfree(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "board";
		create_entity_lockfree<ResourceDecoration, EntityBoard>(name, engine::Asset(name), read_decoration_callback, entity, modelview);
	}

	void create_oven_lockfree(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "oven";
		create_entity_lockfree<ResourceTable, EntityOven>(name, engine::Asset(name), read_table_callback, entity, modelview);
	}

	void create_worker_lockfree(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		constexpr const auto & name = "dude2";
		constexpr engine::Asset asset(name);

		if (resources.contains(asset))
		{
			debug_assert(resources.contains<ResourceCharacter>(asset));

			auto & resource = resources.get<ResourceCharacter>(asset);
			if (resource.is_loading())
			{
				debug_verify(entities.try_emplace<EntityWorker>(entity, modelview));
				resource.add(entity);
			}
			else
			{
				debug_verify(entities.try_emplace<EntityWorker>(entity, entity, resource.get_loaded(), modelview));
				// resource.add(entity);
			}
		}
		else
		{
			debug_verify(entities.try_emplace<EntityWorker>(entity, modelview));
			auto & resource = resources.emplace<ResourceCharacter>(asset);
			resource.add(entity);
			reader->post_read(name, read_character_callback, engine::resource::Format::Placeholder);
			reader->post_read(name, read_armature_callback, engine::resource::Format::Armature);
		}
	}

	void create_placeholder_lockfree(engine::Asset asset, engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		switch (asset)
		{
		case engine::Asset{"bench"}:
			return create_bench_lockfree(entity, modelview);
		case engine::Asset{"board"}:
			return create_board_lockfree(entity, modelview);
		case engine::Asset{"oven"}:
			return create_oven_lockfree(entity, modelview);
		case engine::Asset{"dude1"}:
			debug_printline("create placeholder 'dude1' is deprecated and will be ignored");
			break;
		case engine::Asset{"dude2"}:
			return create_worker_lockfree(entity, modelview);
		default:
			debug_fail();
		}
	}
}

namespace gameplay
{
	void set_dependencies(engine::animation::mixer & mixer_, engine::graphics::renderer & renderer_, engine::physics::simulation & simulation_, engine::resource::reader & reader_, gameplay::gamestate & gamestate_)
	{
		::mixer = &mixer_;
		::renderer = &renderer_;
		::simulation = &simulation_;
		::reader = &reader_;
		::gamestate = &gamestate_;
	}

	void create_level(engine::Entity entity, const std::string & name)
	{
		std::lock_guard<core::sync::CriticalSection> lock(resources_cs);
		create_level_lockfree(entity, name);

		// tmp
		if (!resources.contains(engine::Asset("board")))
		{
			resources.emplace<ResourceDecoration>(engine::Asset("board"));
			reader->post_read("board", read_decoration_callback);
		}
	}

	void create_bench(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		create_bench_lockfree(entity, modelview);
	}

	void create_board(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		create_board_lockfree(entity, modelview);
	}

	void create_oven(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		create_oven_lockfree(entity, modelview);
	}

	void create_worker(engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		create_worker_lockfree(entity, modelview);
	}

	void create_placeholder(engine::Asset asset, engine::Entity entity, const core::maths::Matrix4x4f & modelview)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		create_placeholder_lockfree(asset, entity, modelview);
	}

	void destroy(engine::Entity entity)
	{
		std::lock_guard<core::sync::CriticalSection> load(resources_cs);
		entities.remove(entity);
	}
}
