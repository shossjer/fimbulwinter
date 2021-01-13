#pragma once

#include "core/debug.hpp"

#include "engine/Entity.hpp"

#include <atomic>

namespace engine
{
	class EntityFactory
	{
	private:

		std::atomic<Entity::value_type> next;

	public:

		explicit EntityFactory() : next{1} {} // reserve 0 as a null entity

		explicit EntityFactory(Entity::value_type from) : next{from} { debug_assert(from != 0); } // reserve 0 as a null entity

	public:

		Entity create()
		{
			const auto value = next.fetch_add(1, std::memory_order_relaxed);
			debug_assert(value != 0);

			return Entity(value);
		}
	};
}
