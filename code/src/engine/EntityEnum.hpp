#pragma once

#include "core/debug.hpp"

#include "engine/Entity.hpp"
#include "engine/EntityFactory.hpp"

#include <atomic>

namespace engine
{
	class EntityEnum
	{
	private:

		std::atomic<Entity::value_type> next;

		EntityFactory factory;

	public:

		explicit EntityEnum(EntityFactory && factory) noexcept
			: next{factory.from}
			, factory(static_cast<EntityFactory &&>(factory))
		{}

	public:

		EntityFactory reserve(ful::view_utf8 name, ext::usize count)
		{
			const auto from = next.fetch_add(count, std::memory_order_relaxed);
			const auto remaining = factory.count - (from - factory.from);
			if (debug_assert(count <= remaining)) // todo unnecessary?
			{
				return EntityFactory(from, count, name);
			}
			else
			{
				return EntityFactory(from, remaining, name);
			}
		}

		Entity create()
		{
			const auto value = next.fetch_add(1, std::memory_order_relaxed);
			if (debug_assert(value - factory.from < factory.count))
			{
				return Entity(value);
			}
			else
			{
				return Entity{};
			}
		}

		void reset()
		{
			next.store(factory.from, std::memory_order_relaxed);
		}

	};
}
