#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/concepts.hpp"

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
		constexpr Entity(const value_type id)
			: id{id}
		{}

	public:
		constexpr operator value_type () const
		{
			return id;
		}

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("id"), &Entity::id)
				);
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
		static constexpr Entity null()
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
