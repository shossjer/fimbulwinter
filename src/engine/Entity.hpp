
#ifndef ENGINE_ENTITY_HPP
#define ENGINE_ENTITY_HPP

#include <core/debug.hpp>

#include <cstdint>

namespace engine
{
	class Entity
	{
	public:
		using value_type = uint32_t;
	public:
		static constexpr value_type INVALID = 0;
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
		static Entity create()
		{
			static value_type next_id = 1; // reserve 0 as a special id
			debug_assert(next_id < 0xffffffff);

			return Entity(next_id++);
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
