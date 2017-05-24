
#include "level_placeholder.hpp"

#include <engine/model/loader.hpp>

#include <core/maths/algorithm.hpp>

#include <gameplay/gamestate.hpp>

namespace gameplay
{
namespace level
{
	using Entity = engine::Entity;

	using transform_t = ::engine::transform_t;

	using render_instance_t = engine::graphics::renderer::asset_instance_t;

	engine::Entity load(const placeholder_t & placeholder)
	{
		const engine::Asset asset = placeholder.name;

		// load the model (if not already loaded)
		const engine::model::asset_template_t & assetTemplate =
				engine::model::load(asset, placeholder.name);

		const Entity id = Entity::create();

		switch (asset)
		{
		case engine::Asset{"bench"}:
		{
			const auto & benchDef = assetTemplate.part("bench");
			const auto matrix = placeholder.transform.matrix();

			// register new asset in renderer
			engine::graphics::renderer::add(
				id,
				render_instance_t{
				benchDef.asset,
				matrix });

			// register new asset in gamestate
			gameplay::gamestate::post_add_workstation(
				id,
				gameplay::gamestate::WorkstationType::BENCH,
				matrix*assetTemplate.location("front").transform.matrix());
			break;
		}
		case engine::Asset{ "oven" }:
		{
			const auto & benchDef = assetTemplate.part("oven");
			const auto matrix = placeholder.transform.matrix();

			// register new asset in renderer
			engine::graphics::renderer::add(
				id,
				render_instance_t{
				benchDef.asset,
				matrix });

			// register new asset in gamestate
			gameplay::gamestate::post_add_workstation(
				id,
				gameplay::gamestate::WorkstationType::OVEN,
				matrix*assetTemplate.location("front").transform.matrix());
			break;
		}
		case engine::Asset{ "dude1" }:
		case engine::Asset { "dude2" }:
		{
			const auto & dudeDef = assetTemplate.part("dude");

			// register new asset in renderer
			engine::graphics::renderer::add(
				id,
				render_instance_t {
				dudeDef.asset,
				placeholder.transform.matrix() });

			// register new asset in gamestate
			gameplay::gamestate::post_add_worker(id);
			break;
		}
		default:
			break;
		}

		return id;
	}
}
}
