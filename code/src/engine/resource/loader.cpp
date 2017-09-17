
#include "loader.hpp"

#include <core/async/Thread.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <core/sync/Event.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/Asset.hpp>
#include <engine/Command.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>
#include <engine/resource/reader.hpp>

#include <utility/any.hpp>
#include <utility/string.hpp>
#include <utility/variant.hpp>

#include <unordered_map>

namespace gameplay
{
namespace gamestate
{
	extern void post_command(engine::Entity entity, engine::Command command);
	extern void post_command(engine::Entity entity, engine::Command command, utility::any && data);

	enum class WorkstationType
	{
		BENCH,
		OVEN
	};

	extern void post_add_workstation(
		engine::Entity entity,
		WorkstationType type,
		Matrix4x4f front,
		Matrix4x4f top);

	extern void post_add_worker(engine::Entity entity);
}
}

namespace
{
	// represents model data in file
	// only used when loading a model file. common for all models.
	struct model_t
	{
		struct part_t
		{
			struct render_t
			{
				struct shape_t
				{
					enum class Type
					{
						MESH
					}	type;
					engine::graphics::data::Color color;
					core::container::Buffer vertices;
					core::container::Buffer indices;
					core::container::Buffer normals;
				};

				std::vector<shape_t> shapes;
			};

			struct physic_t
			{
				struct shape_t
				{
					engine::transform_t transform;

					enum class Type
					{
						BOX,
							MESH
							}	type;

					std::vector<Vector3f> points;
					float w, h, d;
				};

				std::vector<shape_t> shapes;
			};

			std::string name;
			render_t render;
			physic_t physic;
		};

		std::vector<part_t> parts;

		struct joint_t
		{
			std::string name;
			engine::transform_t transform;
		};

		std::vector<joint_t> joints;

		struct location_t
		{
			std::string name;
			engine::transform_t transform;
		};

		std::vector<location_t> locations;

		template<typename T>
		const T & get(const std::vector<T> & items, const std::string & name) const
		{
			for (const auto & i : items)
			{
				if (i.name == name) return i;
			}

			debug_unreachable();
		}

		const part_t & part(const std::string & name) const
		{
			return get<part_t>(this->parts, name);
		}

		const joint_t & joint(const std::string & name) const
		{
			return get<joint_t>(this->joints, name);
		}
	};

	engine::transform_t load_transform(const json & jparent)
	{
		const auto & jtransform = jparent["transform"];
		const auto & jp = jtransform["pos"];
		const auto & jq = jtransform["quat"];

		return engine::transform_t{
			Vector3f{ jp[0], jp[1], jp[2] },
				Quaternionf{ jq[0], jq[1], jq[2], jq[3] } };
	}

	model_t load_model_data(const json & jcontent)
	{
		model_t model {};

		const auto & jparts = jcontent["parts"];

		for(const auto & jpart : jparts)
		{
			const std::string name = jpart["name"];
			model_t::part_t::render_t render {};
			{
				const json & jrender = jpart["render"];
				const json & jshapes = jrender["shapes"];

				for (const json & jshape:jshapes)
				{
					model_t::part_t::render_t::shape_t shape;

					const std::string type = jshape["type"];

					if (type == "MESH")
					{
						shape.type = model_t::part_t::render_t::shape_t::Type::MESH;
					}
					else
					{
						debug_unreachable();
					}

					const json & jcolor = jshape["color"];
					const uint32_t r = ((float) jcolor["r"])*255;
					const uint32_t g = ((float) jcolor["g"])*255;
					const uint32_t b = ((float) jcolor["b"])*255;
					shape.color = r + (g<<8) + (b<<16) + (0xff<<24);

					const json & jindices = jshape["indices"];
					const json & jnormals = jshape["normals"];
					const json & jvertices = jshape["vertices"];
					shape.indices.resize<unsigned int>(jindices.size());
					std::copy(jindices.begin(), jindices.end(), shape.indices.data_as<unsigned int>());

					shape.normals.resize<float>(jnormals.size());
					std::copy(jnormals.begin(), jnormals.end(), shape.normals.data_as<float>());

					shape.vertices.resize<float>(jvertices.size());
					std::copy(jvertices.begin(), jvertices.end(), shape.vertices.data_as<float>());

					render.shapes.push_back(shape);
				}
			}

			model_t::part_t::physic_t physic{};
			{
				const json & jphysic = jpart["physic"];
				const json & jshapes = jphysic["shapes"];

				for (const json & jshape : jshapes)
				{
					model_t::part_t::physic_t::shape_t shape;

					shape.transform = load_transform(jshape);

					const std::string type = jshape["type"];

					if (type == "BOX")
					{
						shape.type = model_t::part_t::physic_t::shape_t::Type::BOX;

						const json & jscale = jshape["scale"];

						shape.w = jscale[0];
						shape.h = jscale[1];
						shape.d = jscale[2];
					}
					else
						if (type == "MESH")
						{
							shape.type = model_t::part_t::physic_t::shape_t::Type::MESH;

							// TODO: read all points in the mesh
						}
						else
						{
							debug_unreachable();
						}

					physic.shapes.push_back(shape);
				}
			}

			model.parts.emplace_back(model_t::part_t { name, render, physic });
		}

		const auto & jrelations = jcontent["relations"];

		// read the relations (assume joints only for now)
		for (const auto & jrelation : jrelations)
		{
			const std::string name = jrelation["name"];
			const auto & jjoint = jrelation["joint"];

			model.joints.emplace_back(model_t::joint_t{ name, load_transform(jjoint) });
		}

		const auto & jlocations = jcontent["locations"];

		// read the relations (assume joints only for now)
		for (const auto & jlocation : jlocations)
		{
			const std::string name = jlocation["name"];

			model.locations.emplace_back(model_t::location_t{ name, load_transform(jlocation) });
		}

		return model;
	}

	struct asset_template_t
	{
		struct component_t
		{
			engine::Asset mesh;
			engine::graphics::data::Color color;
		};

		std::unordered_map<
			std::string,
			std::vector<component_t> > components;

		struct joint_t
		{
			engine::transform_t transform;
		};

		std::unordered_map<std::string, joint_t> joints;

		struct location_t
		{
			engine::transform_t transform;
		};

		std::unordered_map<std::string, location_t> locations;

		template<typename T>
		const T & get(
			const std::unordered_map<std::string, T> & items,
			const std::string name) const
		{
			auto itr = items.find(name);

			if (itr == items.end())
			{
				debug_unreachable();
			}

			return itr->second;
		}

		const std::vector<component_t> & part(const std::string name) const
		{
			return get<std::vector<component_t> >(this->components, name);
		}

		const joint_t & joint(const std::string name) const
		{
			return get<joint_t>(this->joints, name);
		}

		const location_t & location(const std::string name) const
		{
			return get<location_t>(this->locations, name);
		}
	};

	const asset_template_t & load(const std::string & modelName, const json & jcontent)
	{
		static std::unordered_map<engine::Asset, asset_template_t> asset_templates;

		auto it = asset_templates.emplace(modelName, asset_template_t{});
		if (!it.second)
			return it.first->second;

		// the asset has not previously been loaded
		asset_template_t & asset_template = it.first->second;

		const model_t model = load_model_data(jcontent);

		for (const auto & part : model.parts)
		{
			std::vector<asset_template_t::component_t> components;
			components.reserve(part.render.shapes.size());

			for (const auto & mesh : part.render.shapes)
			{
				engine::Asset meshId { modelName + part.name + std::to_string(components.size()) };

				engine::graphics::renderer::post_register_mesh(
					meshId,
					engine::graphics::data::Mesh{
						mesh.vertices,
						mesh.indices,
						mesh.normals,
						core::container::Buffer{} });

				components.emplace_back(
					asset_template_t::component_t{ meshId, mesh.color });
			}

			if (!part.physic.shapes.empty())
			{
				// register physics definition of asset

				std::vector<engine::physics::ShapeData> shapes;

				for (const auto & shape : part.physic.shapes)
				{
					engine::physics::ShapeData::Type type;
					engine::physics::ShapeData::Geometry geometry;

					switch (shape.type)
					{
					case model_t::part_t::physic_t::shape_t::Type::BOX:

						type = engine::physics::ShapeData::Type::BOX;
						geometry = engine::physics::ShapeData::Geometry{
							engine::physics::ShapeData::Geometry::Box{ shape.w, shape.h, shape.d*.5f } };
						break;

					default:

						debug_unreachable();
					}

					// TODO: somehow load solidity and material
					shapes.emplace_back(engine::physics::ShapeData{
						type,
						engine::physics::Material::WOOD,
						0.25f,
						shape.transform,
						geometry });
				}
			}

			// save the defined part to be used when creating instances.
			asset_template.components.emplace(part.name, components);
		}

		for (const auto & joint : model.joints)
		{
			asset_template.joints.emplace(joint.name, asset_template_t::joint_t{ joint.transform });
		}

		for (const auto & location : model.locations)
		{
			asset_template.locations.emplace(location.name, asset_template_t::location_t{ location.transform });
		}

		return asset_template;
	}

	auto extract(
		const asset_template_t & assetTemplate,
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
}

namespace
{
	struct MessageLoadLevel;
	struct MessageLoadPlaceholder;
	struct MessageReadLevel;
	struct MessageReadPlaceholder;

	template <typename MessageLoad, typename MessageRead, typename Data>
	struct Asset;

	using AssetLevel = Asset<MessageLoadLevel, MessageReadLevel, engine::resource::data::Level>;
	using AssetPlaceholder = Asset<MessageLoadPlaceholder, MessageReadPlaceholder, engine::resource::data::Placeholder>;

	template <typename MessageType, typename ...Ps>
	void post_message(Ps && ...ps);
}

namespace
{
	struct Progression
	{
		int count;
		int parent;
	};
	std::unordered_map<int, Progression> progressions;
	int next_id = 1; // 0 is reserved

	void increase_count(int id)
	{
		auto maybe = progressions.find(id);
		debug_assert(maybe != progressions.end());

		maybe->second.count++;
	}

	int create_child(int parent)
	{
		if (parent != 0)
			increase_count(parent);

		auto maybe = progressions.emplace(next_id++, Progression{0, parent});
		return maybe.first->first;
	}
	int create_root()
	{
		return create_child(0);
	}

	bool decrease_count(int id)
	{
		auto maybe = progressions.find(id);
		debug_assert(maybe != progressions.end());
		debug_assert(maybe->second.count > 0);

		maybe->second.count--;
		return maybe->second.count == 0;
	}

	void destroy_child(engine::Entity entity, int id)
	{
		auto maybe = progressions.find(id);
		debug_assert(maybe != progressions.end());
		debug_assert(maybe->second.count == 0);

		if (maybe->second.parent == 0)
		{
			gameplay::gamestate::post_command(entity, engine::Command::LOADER_FINISHED);
		}
		else if (decrease_count(maybe->second.parent))
		{
			destroy_child(entity, maybe->second.parent);
		}
	}
}

namespace
{
	struct MessageLoadInfo
	{
		engine::Entity entity;
		int id;
	};

	struct MessageLoadLevel
	{
		using AssetType = AssetLevel;

		MessageLoadInfo info;
		engine::external::loader::Level data;
	};
	struct MessageLoadPlaceholder
	{
		using AssetType = AssetPlaceholder;

		MessageLoadInfo info;
		engine::external::loader::Placeholder data;
	};
	struct MessageReadLevel
	{
		using AssetType = AssetLevel;

		engine::Asset asset;
		engine::resource::data::Level data;
	};
	struct MessageReadPlaceholder
	{
		using AssetType = AssetPlaceholder;

		engine::Asset asset;
		engine::resource::data::Placeholder data;
	};
	using Message = utility::variant
	<
		MessageLoadLevel,
		MessageLoadPlaceholder,
		MessageReadLevel,
		MessageReadPlaceholder
	>;

	core::container::CircleQueueSRMW<Message, 100> queue_messages;
}

namespace
{
	void post_read(const std::string & filename, void (* callback)(const std::string &, engine::resource::data::Level && data))
	{
		engine::resource::post_read_level(filename, callback);
	}
	void post_read(const std::string & filename, void (* callback)(const std::string &, engine::resource::data::Placeholder && data))
	{
		engine::resource::post_read_placeholder(filename, callback);
	}

	void create_asset(const MessageLoadInfo & info, const engine::resource::data::Level & level)
	{
		engine::physics::camera::set(engine::physics::camera::Bounds{
				Vector3f{-6.f, 0.f, -4.f},
				Vector3f{6.f, 10.f, 12.f} });

		for (const auto & data : level.meshes)
		{
			const engine::Asset asset = data.name;

			engine::graphics::renderer::post_register_mesh(
				asset,
				engine::graphics::data::Mesh{
					data.vertices,
						data.triangles,
						data.normals,
						core::container::Buffer{} });

			std::vector<engine::graphics::data::CompC::asset> assets;

			const uint32_t r = data.color[0] * 255;
			const uint32_t g = data.color[1] * 255;
			const uint32_t b = data.color[2] * 255;
			assets.emplace_back(
				engine::graphics::data::CompC::asset{
					asset,
						r + (g << 8) + (b << 16) + (0xff << 24) });

			const auto entity = engine::Entity::create();
			engine::graphics::renderer::post_add_component(
				entity,
				engine::graphics::data::CompC{
					data.matrix,
						Vector3f{1.f, 1.f, 1.f},
						assets});
			engine::graphics::renderer::post_make_obstruction(entity);
		}

		for (auto & placeholder : level.placeholders)
		{
			post_message<MessageLoadPlaceholder>(MessageLoadInfo{info.entity, create_child(info.id)}, engine::external::loader::Placeholder{placeholder.name, placeholder.translation, placeholder.rotation, placeholder.scale});
		}
	}
	void create_entity(MessageLoadLevel && x, const engine::resource::data::Level & level)
	{
	}

	void create_asset(const MessageLoadInfo & info, const engine::resource::data::Placeholder & data)
	{
		struct CreateAsset
		{
			const engine::resource::data::Placeholder::Header & header;

			void operator () (const engine::model::mesh_t & data)
			{
				engine::graphics::renderer::post_register_character(header.asset, engine::model::mesh_t(data));
			}
			void operator () (const json & data)
			{
			}
		};
		visit(CreateAsset{data.header}, data.data);
	}
	void create_entity(MessageLoadPlaceholder && x, const engine::resource::data::Placeholder & data)
	{
		struct CreateEntity
		{
			MessageLoadPlaceholder && x;
			const engine::resource::data::Placeholder::Header & header;

			void operator () (const engine::model::mesh_t & data)
			{
				engine::Entity id = engine::Entity::create();

				engine::graphics::renderer::post_add_character(
					id, engine::graphics::data::CompT{
						make_translation_matrix(x.data.translation) *
							make_matrix(x.data.rotation),
							Vector3f { 1.f, 1.f, 1.f },
							header.asset,
								engine::Asset{ "dude" }});
				engine::graphics::renderer::post_make_selectable(id);
				engine::physics::post_add_object(id, engine::transform_t{x.data.translation, x.data.rotation});
				engine::animation::add(id, engine::animation::armature{utility::to_string("res/", header.name, ".arm")});
				engine::animation::update(id, engine::animation::action{"turn-left", true});

				// register new asset in gamestate
				gameplay::gamestate::post_add_worker(id);
			}
			void operator () (const json & data)
			{
				const asset_template_t & assetTemplate = load(header.name, data);

				const core::maths::Matrix4x4f matrix =
					make_translation_matrix(x.data.translation) *
					make_matrix(x.data.rotation);

				switch (header.asset)
				{
				case engine::Asset{ "bench" }:
				{
					const auto & assets = extract(assetTemplate, "bench");

					const engine::Entity id = engine::Entity::create();
					// register new asset in renderer
					engine::graphics::renderer::post_add_component(
						id,
						engine::graphics::data::CompC{
							matrix,
								Vector3f{ 1.f, 1.f, 1.f },
								assets });
					engine::graphics::renderer::post_make_selectable(id);

					// register new asset in gamestate
					gameplay::gamestate::post_add_workstation(
						id,
						gameplay::gamestate::WorkstationType::BENCH,
						matrix * assetTemplate.location("front").transform.matrix(),
						matrix * assetTemplate.location("top").transform.matrix());
					break;
				}
				case engine::Asset{ "oven" }:
				{
					const auto & assets = extract(assetTemplate, "oven");

					const engine::Entity id = engine::Entity::create();
					// register new asset in renderer
					engine::graphics::renderer::post_add_component(
						id,
						engine::graphics::data::CompC{
							matrix,
								Vector3f{ 1.f, 1.f, 1.f },
								assets });
					engine::graphics::renderer::post_make_selectable(id);

					// register new asset in gamestate
					gameplay::gamestate::post_add_workstation(
						id,
						gameplay::gamestate::WorkstationType::OVEN,
						matrix * assetTemplate.location("front").transform.matrix(),
						matrix * assetTemplate.location("top").transform.matrix());
					break;
				}
				case engine::Asset{ "dude1" }:
				case engine::Asset{ "dude2" }:
				{
					const auto & assets = extract(assetTemplate, "dude");

					const engine::Entity id = engine::Entity::create();
					// register new asset in renderer
					engine::graphics::renderer::post_add_component(
						id,
						engine::graphics::data::CompC{
							matrix,
								Vector3f{ 1.f, 1.f, 1.f },
								assets });
					engine::graphics::renderer::post_make_obstruction(id);

					// register new asset in gamestate
					gameplay::gamestate::post_add_worker(id);
					break;
				}
				case engine::Asset{ "board" }:
				{
					const auto & assets = extract(assetTemplate, "all");

					const engine::Entity id = engine::Entity::create();
					// register new asset in renderer
					engine::graphics::renderer::post_add_component(
						id,
						engine::graphics::data::CompC{
							matrix,
								Vector3f{ 1.f, 1.f, 1.f },
								assets });

					break;
				}
				default:
					break;
				}
			}
		};
		visit(CreateEntity{std::move(x), data.header}, data.data);

		destroy_child(x.info.entity, x.info.id);
	}

	template <typename MessageRead, typename Data>
	void read_callback(const std::string & filename, Data && data);

	template <typename MessageLoad, typename MessageRead, typename Data>
	struct Asset
	{
		int refcount;
		bool ready_to_use;

		MessageLoadInfo info;
		Data data;

		std::vector<MessageLoad> xs;

		Asset(const MessageLoad & x)
			: refcount(0)
			, ready_to_use(false)
			, info(x.info)
		{
			post_read(x.data.name, read_callback<MessageRead, Data>);
		}

		void load(MessageLoad && x)
		{
			refcount++;
			if (ready_to_use)
			{
				create_entity(std::move(x), data);
			}
			else
			{
				xs.push_back(std::move(x));
			}
		}

		void set(Data && data)
		{
			ready_to_use = true;
			this->data = std::move(data);
			create_asset(info, this->data);

			for (auto & x : xs)
			{
				create_entity(std::move(x), this->data);
			}
			xs.clear();
		}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		205,
		std::array<AssetLevel, 2>,
		std::array<AssetPlaceholder, 100>
	> assets;
}

namespace
{
	struct ProcessMessage
	{
		template <typename T>
		auto operator () (T && x) -> decltype((void)x.info)
		{
			engine::Asset asset(x.data.name);
			if (assets.contains(asset))
			{
				assets.get<typename T::AssetType>(asset).load(std::forward<T>(x));
			}
			else
			{
				auto & kjhs = assets.emplace<typename T::AssetType>(asset, x);
				kjhs.load(std::forward<T>(x));
			}
		}
		template <typename T>
		auto operator () (T && x) -> decltype((void)x.asset)
		{
			if (!assets.contains(x.asset))
			{
				debug_printline(0xffffffff, "We have read an asset that is not used any more");
				debug_fail();
			}
			auto & kjhs = assets.get<typename T::AssetType>(x.asset);
			kjhs.set(std::move(x.data));
		}
	};

	void loader_load()
	{
		Message message;
		while (queue_messages.try_pop(message))
		{
			visit(ProcessMessage{}, std::move(message));
		}
	}

	core::async::Thread renderThread;
	volatile bool active = false;
	core::sync::Event<true> event;

	void run()
	{
		event.wait();
		event.reset();

		while (active)
		{
			loader_load();

			event.wait();
			event.reset();
		}
	}

	template <typename MessageType, typename ...Ps>
	void post_message(Ps && ...ps)
	{
		const auto res = queue_messages.try_emplace(utility::in_place_type<MessageType>, std::forward<Ps>(ps)...);
		debug_assert(res);
		if (res)
		{
			event.set();
		}
	}

	template <typename MessageRead, typename Data>
	void read_callback(const std::string & filename, Data && data)
	{
		post_message<MessageRead>(filename, std::move(data));
	}
}

namespace engine
{
	namespace external
	{
		void create()
		{
			debug_assert(!active);

			active = true;
			renderThread = core::async::Thread{ run };
		}

		void destroy()
		{
			debug_assert(active);

			active = false;
			event.set();

			renderThread.join();
		}

		void post_load_level(engine::Entity entity, loader::Level && data)
		{
			post_message<MessageLoadLevel>(MessageLoadInfo{entity, create_root()}, std::move(data));
		}
		void post_load_placeholder(engine::Entity entity, loader::Placeholder && data)
		{
			post_message<MessageLoadPlaceholder>(MessageLoadInfo{entity, create_root()}, std::move(data));
		}
	}
}
