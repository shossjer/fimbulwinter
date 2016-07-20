
#include "effects.hpp"

#include "Effect.hpp"

#include <engine/physics/effects.hpp>
#include <engine/physics/physics.hpp>

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

namespace gameplay
{
namespace effects
{
	class PlayerGravity : public Effect
	{
	private:

		const unsigned int callerId;

	public:

		PlayerGravity(const unsigned int id) : callerId(id) {}

	public:

		bool finished() override
		{
			return false;
		}

		void update() override
		{
			// get centre position
			const engine::physics::Point point = engine::physics::load(callerId);

			// get mouse coordinates in the world

			// update physics

			engine::physics::effect::acceleration(
				::engine::physics::Vector{ 0.f, 20.f, 0.f }, 
				point,
				10.f);
		}
	};

	namespace
	{
		std::unordered_map<Id, std::unique_ptr<Effect> > items;
	}

	void update()
	{
		// create 

		// remove

		// update
		for (auto i = items.begin(); i!= items.end();)
		{
			(*i).second->update();

			if ((*i).second->finished())
			{
				i = items.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	Id create(const Type type, const Id callerId)
	{
		printf("Player ability CREATING!\n");

		static std::atomic<unsigned int> seed{ 1 };

		const unsigned int id = seed++;

		switch (type)
		{
		case Type::PLAYER_GRAVITY:

			items.emplace(id, std::unique_ptr<Effect>(new PlayerGravity(callerId)));
			break;
		}

		return id;
	}

	void remove(const Id id)
	{
		printf("Player ability REMOVING!\n");

		items.erase(id);
	}
}
}
