
#ifndef ENGINE_PHYSICS_WORLD_HPP
#define ENGINE_PHYSICS_WORLD_HPP

#include "defines.hpp"
#include "MaterialData.hpp"
#include "Callbacks.hpp"

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>

#include <unordered_map>

namespace engine
{
namespace physics
{
	template <class T> class WorldBase
	{
	public:

		const float timeStep;

		struct Actor
		{
			T *const body;
			::engine::Entity debugRenderId;
			/**
			* Rotation along the y-axis (yaw).
			*
			* This value  is not used  by the underlaying  2D physics
			* since  it  only  handles  rotations  along  the  z-axis
			* (roll). It is however used in the modelview matrix sent
			* to the renderer to help create the illusion of 3D.
			*/
			core::maths::radianf heading;

			Actor(T *const body, const ::engine::Entity debugRenderId)
				:
				body(body),
				debugRenderId(debugRenderId),
				heading(0.f)
			{
			}
		};

	protected:

		// Contains all created physics Actors in the world.
		// Accessed through
		//	* getActor
		//	* getBody
		std::unordered_map<engine::Entity, Actor> actors;

		// Callback instance for engine notifications
		//	* onFalling - called when Actor start falling
		//	* onGrounded - called when Actor is grounded after falling
		const Callbacks * callbacks;

	public:

		WorldBase(const float timeStep) : timeStep(timeStep) {}

	public:

		Actor & getActor(const ::engine::Entity id) { return this->actors.at(id); }

		const Actor & getActor(const ::engine::Entity id) const { return this->actors.at(id); }

		T * getBody(const ::engine::Entity id) { return getActor(id).body; }

		const T * getBody(const ::engine::Entity id) const { return getActor(id).body; }

	protected:

		Actor & create(const ::engine::Entity id, T * const body)
		{
			this->actors.emplace(id, Actor {body, ::engine::Entity::INVALID});
			return this->actors.at(id);
		}
	};
}
}

#endif // ENGINE_PHYSICS_WORLD_HPP