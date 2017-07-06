
#include "level_placeholder.hpp"

#include <engine/model/loader.hpp>

#include <core/maths/algorithm.hpp>

#include <gameplay/gamestate.hpp>

#include <fstream>

namespace gameplay
{
namespace level
{
	using Entity = engine::Entity;

	using transform_t = ::engine::transform_t;

	using render_instance_t = engine::graphics::renderer::asset_instance_t;

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
			const auto & benchDef = assetTemplate.part("bench");

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
				matrix*assetTemplate.location("front").transform.matrix(),
				matrix*assetTemplate.location("top").transform.matrix());
			break;
		}
		case engine::Asset{ "oven" }:
		{
			const auto & benchDef = assetTemplate.part("oven");

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
				matrix*assetTemplate.location("front").transform.matrix(),
				matrix*assetTemplate.location("top").transform.matrix());
			break;
		}
		case engine::Asset{ "dude1" }:
		case engine::Asset{ "dude2" }:
		{
			const auto & dudeDef = assetTemplate.part("dude");

			// register new asset in renderer
			engine::graphics::renderer::add(
				id,
				render_instance_t{
				dudeDef.asset,
				matrix });

			// register new asset in gamestate
			gameplay::gamestate::post_add_worker(id);
			break;
		}
		case engine::Asset{ "board" }:
		{
			const auto & dudeDef = assetTemplate.part("all");

			// register new asset in renderer
			engine::graphics::renderer::add(
				id,
				render_instance_t{
				dudeDef.asset,
				matrix });

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
			engine::graphics::renderer::add_character_instance(
				id, render_instance_t{
					engine::Asset{placeholder.name},
					placeholder.transform.matrix() });
		}
		else
		{
			load(id, placeholder.name, placeholder.transform.matrix());
		}

		return id;
	}
}
}
