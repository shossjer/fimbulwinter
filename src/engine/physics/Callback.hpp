 
#ifndef ENGINE_PHYSICS_CALLBACK_HPP
#define ENGINE_PHYSICS_CALLBACK_HPP

#include <engine/Entity.hpp>
#include <engine/physics/defines.hpp>

#include <core/maths/Vector.hpp>

namespace engine
{
namespace physics
{
	class Callback
	{
	public:
		/**
			\brief Post update from Physics simulation when new contact is found.
			\note Called during physics Simulation. Only use this thread to put the data on queue for other thread.

			\param ids of the two Actors
			\param material of each of the Shape in the contact.
		 */
		virtual void postContactFound(const Entity ids[2], const Material materials[2]) const = 0;
		/**
			\brief Post update from Physics simulation when existing contact is lost
			\note Called during physics Simulation. Only use this thread to put the data on queue for other thread.

			\param ids of the two Actors
		 */
		virtual void postContactLost(const Entity ids[2]) const = 0;
	};
}
}

#endif /* ENGINE_PHYSICS_CALLBACKS_HPP */
