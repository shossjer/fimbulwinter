
#ifndef ENGINE_ENTITY_HPP
#define ENGINE_ENTITY_HPP

#include <core/debug.hpp>
#include "core/serialize.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <ostream>

namespace engine
{
	class Entity
	{
	public:
		using value_type = uint32_t;

	private:
		using this_type = Entity;

	private:
		value_type id;

	public:
		Entity() = default;
	// private:
		// This constructor  should be private to  prevent anyone from
		// creating any  numbered entity, but  there is a lot  of code
		// now that depends on the fact  that entities are the same as
		// regular integers so that needs to be fixed first.
		Entity(const value_type id) :
			id{id}
		{
		}

	public:
		operator value_type () const
		{
			return this->id;
		}

	public:
		template <typename S>
		friend void serialize(S & s, this_type x)
		{
			using core::serialize;

			serialize(s, x.id);
		}

		friend std::ostream & operator << (std::ostream & stream, this_type x)
		{
			stream << x.id;
			return stream;
		}

	public:
		static Entity create()
		{
			static std::atomic<value_type> next_id{1}; // reserve 0 as a special id

			const auto id = next_id.fetch_add(1, std::memory_order_relaxed);
			debug_assert(id < 0xffffffff);

			return Entity{id};
		}
		static Entity null()
		{
			return Entity{0};
		}
	};
}

namespace std
{
	template<> struct hash<engine::Entity>
	{
		std::size_t operator () (const engine::Entity entity) const
		{
			return std::hash<std::size_t>{}(entity);
		}
	};
}

#endif /* ENGINE_ENTITY_HPP */
