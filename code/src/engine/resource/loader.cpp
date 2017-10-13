
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

	const engine::resource::loader::asset_template_t & load(const std::string & modelName, const json & jcontent)
	{
		static std::unordered_map<engine::Asset, engine::resource::loader::asset_template_t> asset_templates;

		auto it = asset_templates.emplace(modelName, engine::resource::loader::asset_template_t{});
		if (!it.second)
			return it.first->second;

		// the asset has not previously been loaded
		engine::resource::loader::asset_template_t & asset_template = it.first->second;

		const model_t model = load_model_data(jcontent);

		for (const auto & part : model.parts)
		{
			std::vector<engine::resource::loader::asset_template_t::component_t> components;
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
					engine::resource::loader::asset_template_t::component_t{ meshId, mesh.color });
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
			asset_template.joints.emplace(joint.name, engine::resource::loader::asset_template_t::joint_t{ joint.transform });
		}

		for (const auto & location : model.locations)
		{
			asset_template.locations.emplace(location.name, engine::resource::loader::asset_template_t::location_t{ location.transform });
		}

		return asset_template;
	}
}

namespace
{
	template <typename MessageType, typename ...Ps>
	void post_message(Ps && ...ps);
}

namespace
{
	struct MessageLoadLevel
	{
		std::string name;
		void (* callback)(std::string name, const engine::resource::loader::Level & data);
	};
	struct MessageLoadPlaceholder
	{
		std::string name;
		void (* callback)(std::string name, const engine::resource::loader::Placeholder & data);
	};
	struct MessageReadLevel
	{
		std::string name;
		engine::resource::reader::Level data;
	};
	struct MessageReadPlaceholder
	{
		std::string name;
		engine::resource::reader::Placeholder data;
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
	engine::resource::loader::Level create_asset(std::string && name, engine::resource::reader::Level && data)
	{
		engine::physics::camera::set(engine::physics::camera::Bounds{
				Vector3f{-6.f, 0.f, -4.f},
				Vector3f{6.f, 10.f, 12.f} });

		engine::resource::loader::Level level;

		level.meshes.reserve(data.meshes.size());
		for (const auto & mesh : data.meshes)
		{
			const engine::Asset asset = mesh.name;

			engine::graphics::renderer::post_register_mesh(
				asset,
				engine::graphics::data::Mesh{
					mesh.vertices,
						mesh.triangles,
						mesh.normals,
						core::container::Buffer{} });

			const uint32_t r = mesh.color[0] * 255;
			const uint32_t g = mesh.color[1] * 255;
			const uint32_t b = mesh.color[2] * 255;
			const auto color = r + (g << 8) + (b << 16) + (0xff << 24);

			level.meshes.push_back(engine::resource::loader::Level::Mesh{mesh.name, mesh.matrix, color});
		}

		level.placeholders.reserve(data.placeholders.size());
		for (const auto & placeholder : data.placeholders)
		{
			level.placeholders.push_back(engine::resource::loader::Level::Placeholder{placeholder.name, placeholder.translation, placeholder.rotation, placeholder.scale});
		}

		return level;
	}

	engine::resource::loader::Placeholder create_asset(std::string && name, engine::resource::reader::Placeholder && data)
	{
		struct CreateAsset
		{
			std::string && name;

			engine::resource::loader::Placeholder operator () (engine::model::mesh_t && data)
			{
				debug_printline(0xffffffff, "registering character \"", name, "\"");
				engine::graphics::renderer::post_register_character(std::move(name), std::move(data));

				return engine::resource::loader::Placeholder();
			}
			engine::resource::loader::Placeholder operator () (json && data)
			{
				const engine::resource::loader::asset_template_t & assetTemplate = load(std::move(name), std::move(data));

				return engine::resource::loader::Placeholder(assetTemplate);
			}
		};
		return visit(CreateAsset{std::move(name)}, std::move(data.data));
	}

	struct AssetLevel
	{
		struct Loaded
		{
			engine::resource::loader::Level data;

			Loaded(engine::resource::loader::Level && data)
				: data(std::move(data))
			{}
		};
		struct Loading
		{
			std::vector<MessageLoadLevel> messages;

			Loading(MessageLoadLevel && message)
			{
				messages.push_back(std::move(message));
			}
		};
		using State = utility::variant<Loading, Loaded>;

		State state;

		AssetLevel(MessageLoadLevel && message)
			: state(utility::in_place_type<Loading>, std::move(message))
		{}

		void call_or_add(MessageLoadLevel && message)
		{
			struct AddOrCall
			{
				MessageLoadLevel && message;

				void operator () (const Loaded & x)
				{
					message.callback(message.name, x.data);
				}
				void operator () (Loading & x)
				{
					x.messages.push_back(std::move(message));
				}
			};
			visit(AddOrCall{std::move(message)}, state);
		}

		void load(std::string && name, engine::resource::reader::Level && data)
		{
			debug_assert(utility::holds_alternative<Loading>(state));

			std::vector<MessageLoadLevel> messages = std::move(utility::get<Loading>(state).messages);

			const Loaded & loaded = state.emplace<Loaded>(create_asset(std::move(name), std::move(data)));

			for (const auto & message : messages)
			{
				message.callback(message.name, loaded.data);
			}
		}
	};
	struct AssetPlaceholder
	{
		struct Loaded
		{
			engine::resource::loader::Placeholder data;

			Loaded(engine::resource::loader::Placeholder && data)
				: data(std::move(data))
			{}
		};
		struct Loading
		{
			std::vector<MessageLoadPlaceholder> messages;

			Loading(MessageLoadPlaceholder && message)
			{
				messages.push_back(std::move(message));
			}
		};
		using State = utility::variant<Loading, Loaded>;

		State state;

		AssetPlaceholder(MessageLoadPlaceholder && message)
			: state(utility::in_place_type<Loading>, std::move(message))
		{}

		void call_or_add(MessageLoadPlaceholder && message)
		{
			struct AddOrCall
			{
				MessageLoadPlaceholder && message;

				void operator () (const Loaded & x)
				{
					message.callback(message.name, x.data);
				}
				void operator () (Loading & x)
				{
					x.messages.push_back(std::move(message));
				}
			};
			visit(AddOrCall{std::move(message)}, state);
		}

		void load(std::string && name, engine::resource::reader::Placeholder && data)
		{
			debug_assert(utility::holds_alternative<Loading>(state));

			std::vector<MessageLoadPlaceholder> messages = std::move(utility::get<Loading>(state).messages);

			const Loaded & loaded = state.emplace<Loaded>(create_asset(std::move(name), std::move(data)));

			for (const auto & message : messages)
			{
				message.callback(message.name, loaded.data);
			}
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
	template <typename MessageRead>
	void read_callback(std::string name, decltype(std::declval<MessageRead>().data) && data);

	void loader_load()
	{
		Message message;
		while (queue_messages.try_pop(message))
		{
			struct ProcessMessage
			{
				void operator () (MessageLoadLevel && x)
				{
					const engine::Asset asset = x.name;
					if (assets.contains(asset))
					{
						debug_assert(assets.contains<AssetLevel>(asset));

						assets.get<AssetLevel>(asset).call_or_add(std::move(x));
					}
					else
					{
						engine::resource::reader::post_read_level(x.name, read_callback<MessageReadLevel>);
						assets.emplace<AssetLevel>(asset, std::move(x));
					}
				}
				void operator () (MessageLoadPlaceholder && x)
				{
					const engine::Asset asset = x.name;
					if (assets.contains(asset))
					{
						debug_assert(assets.contains<AssetPlaceholder>(asset));

						assets.get<AssetPlaceholder>(asset).call_or_add(std::move(x));
					}
					else
					{
						engine::resource::reader::post_read_placeholder(x.name, read_callback<MessageReadPlaceholder>);
						assets.emplace<AssetPlaceholder>(asset, std::move(x));
					}
				}
				void operator () (MessageReadLevel && x)
				{
					const engine::Asset asset = x.name;
					if (!assets.contains(asset))
					{
						debug_printline(0xffffffff, "We have read an asset that is not used any more");
						debug_fail();
					}
					debug_assert(assets.contains<AssetLevel>(asset));

					assets.get<AssetLevel>(asset).load(std::move(x.name), std::move(x.data));
				}
				void operator () (MessageReadPlaceholder && x)
				{
					const engine::Asset asset = x.name;
					if (!assets.contains(asset))
					{
						debug_printline(0xffffffff, "We have read an asset that is not used any more");
						debug_fail();
					}
					debug_assert(assets.contains<AssetPlaceholder>(asset));

					assets.get<AssetPlaceholder>(asset).load(std::move(x.name), std::move(x.data));
				}
			};
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

	template <typename MessageRead>
	void read_callback(std::string name, decltype(std::declval<MessageRead>().data) && data)
	{
		post_message<MessageRead>(std::move(name), std::move(data));
	}
}

namespace engine
{
	namespace resource
	{
		namespace loader
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

			void post_load_level(std::string name, void (* callback)(std::string name, const Level & data))
			{
				post_message<MessageLoadLevel>(std::move(name), callback);
			}
			void post_load_placeholder(std::string name, void (* callback)(std::string name, const Placeholder & data))
			{
				post_message<MessageLoadPlaceholder>(std::move(name), callback);
			}
		}
	}
}
