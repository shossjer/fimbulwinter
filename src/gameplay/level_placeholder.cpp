
#include "level_placeholder.hpp"

#include <engine/animation/mixer.hpp>
#include <engine/model/loader.hpp>

#include <core/maths/algorithm.hpp>

#include <gameplay/gamestate.hpp>

#include <utility/string.hpp>

#include <fstream>

namespace gameplay
{
namespace level
{
	using Entity = engine::Entity;

	using transform_t = ::engine::transform_t;

	using CompC = engine::graphics::data::CompC;
	using CompT = engine::graphics::data::CompT;

	auto extract(
		const engine::model::asset_template_t & assetTemplate,
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

	void load(const engine::Entity id, const std::string & name, const Matrix4x4f & matrix)
	{
		const engine::Asset asset = name;

		// load the model (if not already loaded)
		const engine::model::asset_template_t & assetTemplate =
			engine::model::load(asset, name);

		switch (asset)
		{
		case engine::Asset{ "bench" }:
		{
			const auto & assets = extract(assetTemplate, "bench");

			// register new asset in renderer
			engine::graphics::renderer::post_add_component(
				id,
				CompC{
					matrix,
					Vector3f{ 1.f, 1.f, 1.f },
					assets });
			engine::graphics::renderer::post_make_selectable(id);

			// register new asset in gamestate
			gameplay::gamestate::post_add_workstation(
				id,
				gameplay::gamestate::WorkstationType::BENCH,
				matrix*assetTemplate.location("front").transform.matrix(),
				matrix*assetTemplate.location("top").transform.matrix());
			break;
		}
		case engine::Asset{ "oven" }:
		{
			const auto & assets = extract(assetTemplate, "oven");

			// register new asset in renderer
			engine::graphics::renderer::post_add_component(
				id,
				CompC{
					matrix,
					Vector3f{ 1.f, 1.f, 1.f },
					assets });
			engine::graphics::renderer::post_make_selectable(id);

			// register new asset in gamestate
			gameplay::gamestate::post_add_workstation(
				id,
				gameplay::gamestate::WorkstationType::OVEN,
				matrix*assetTemplate.location("front").transform.matrix(),
				matrix*assetTemplate.location("top").transform.matrix());
			break;
		}
		case engine::Asset{ "dude1" }:
		case engine::Asset{ "dude2" }:
		{
			const auto & assets = extract(assetTemplate, "dude");

			// register new asset in renderer
			engine::graphics::renderer::post_add_component(
				id,
				CompC{
					matrix,
					Vector3f{ 1.f, 1.f, 1.f },
					assets });

			// register new asset in gamestate
			gameplay::gamestate::post_add_worker(id);
			break;
		}
		case engine::Asset{ "board" }:
		{
			const auto & assets = extract(assetTemplate, "all");

			// register new asset in renderer
			engine::graphics::renderer::post_add_component(
				id,
				CompC{
					matrix,
					Vector3f{ 1.f, 1.f, 1.f },
					assets });

			break;
		}
		default:
			break;
		}
	}

	engine::Entity load(const placeholder_t & placeholder)
	{
		const Entity id = Entity::create();

		// load as binary if available
		if (engine::model::load_binary(placeholder.name))
		{
			engine::graphics::renderer::post_add_character(
				id, CompT {
					placeholder.transform.matrix(),
					Vector3f { 1.f, 1.f, 1.f },
					engine::Asset{placeholder.name},
					engine::Asset{ "dude" }});
			engine::graphics::renderer::post_make_selectable(id);
			engine::animation::add(id, engine::animation::armature{utility::to_string("res/", placeholder.name, ".arm")});
			engine::animation::update(id, engine::animation::action{"idle", true});

			// register new asset in gamestate
			gameplay::gamestate::post_add_worker(id);
		}
		else
		{
			load(id, placeholder.name, placeholder.transform.matrix());
		}

		return id;
	}
}
}
