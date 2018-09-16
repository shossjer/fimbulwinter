 
#ifndef ENGINE_PHYSICS_CALLBACK_HPP
#define ENGINE_PHYSICS_CALLBACK_HPP

#include "engine/Entity.hpp"
#include "engine/common.hpp"
#include "engine/physics/defines.hpp"

#include <array>

namespace engine
{
namespace physics
{
	class Callback
	{
	public:

		struct data_t
		{
			Entity ids[2];
			ActorData::Behaviour behaviours[2];
			Material materials[2];
		};

	public:
		/**
			\brief Post update from Physics simulation when new contact is found.
			\note Called during physics Simulation. Only use this thread to put the data on queue for other thread.

			The first Actor will always be the more prio object according to Behaviour.

			\param ids of the two Actors
			\param behaviours of the two Actors
			\param material of each of the Shape in the contact.
		 */
		virtual void postContactFound(const data_t & data) const = 0;

		/**
			\brief Post update from Physics simulation when existing contact is lost
			\note Called during physics Simulation. Only use this thread to put the data on queue for other thread.

			The first Actor will always be the more prio object according to Behaviour.

			\param ids of the two Actors
			\param behaviours of the two Actors
			\param material of each of the Shape in the contact.
		 */
		virtual void postContactLost(const data_t & data) const = 0;

		/**
			\brief Same as postContactFound but first element will always be a trigger
		 */
		virtual void postTriggerFound(const data_t & data) const = 0;

		/**
			\brief Same as postContactLost but first element will always be a trigger
		 */
		virtual void postTriggerLost(const data_t & data) const = 0;

		virtual void postTransformation(
				const Entity id,
				const transform_t & translation) const = 0;
	};
}
}

#endif /* ENGINE_PHYSICS_CALLBACKS_HPP */
