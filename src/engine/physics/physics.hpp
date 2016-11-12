
#ifndef ENGINE_PHYSICS_PHYSICS_HPP
#define ENGINE_PHYSICS_PHYSICS_HPP

#include "defines.hpp"

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>

#include <array>
#include <vector>

namespace engine
{
namespace physics
{
	/**
	 *	steps physics engine forward
	 */
	void update();

	/**
	 */
	void post_movement(engine::Entity id, core::maths::Vector3f movement);
	/**
	 */
	void post_set_heading(engine::Entity id, core::maths::radianf rotation);

	void create(const engine::Entity id, const BoxData & data);

	void create(const engine::Entity id, const CharacterData & data);

	void create(const engine::Entity id, const CylinderData & data);

	void create(const engine::Entity id, const SphereData & data);

	void remove(const engine::Entity id);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
