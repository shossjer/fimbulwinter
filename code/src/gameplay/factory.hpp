
#ifndef GAMEPLAY_FACTORY_HPP
#define GAMEPLAY_FACTORY_HPP

#include "core/maths/Matrix.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"

namespace gameplay
{
	void create_bench(engine::Entity entity, const core::maths::Matrix4x4f & modelview);
	void create_board(engine::Entity entity, const core::maths::Matrix4x4f & modelview);
	void create_oven(engine::Entity entity, const core::maths::Matrix4x4f & modelview);
	// void create_static(engine::Entity entity);
	// void create_worker_deprecated(engine::Entity entity);
	void create_worker(engine::Entity entity, const core::maths::Matrix4x4f & modelview);

	void create_level(engine::Entity entity, const std::string & name);

	void create_placeholder(engine::Asset asset, engine::Entity entity, const core::maths::Matrix4x4f & modelview);

	void destroy(engine::Entity entity);
}

#endif /* GAMEPLAY_FACTORY_HPP */
