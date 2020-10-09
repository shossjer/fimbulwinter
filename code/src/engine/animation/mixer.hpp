#pragma once

#include "core/container/Buffer.hpp"
#include "core/serialization.hpp"

#include "engine/animation/Armature.hpp"
#include "engine/common.hpp"
#include "engine/Token.hpp"

#include <vector>

namespace engine
{
	namespace graphics
	{
		class renderer;
	}

	namespace physics
	{
		class simulation;
	}
}

namespace engine
{
	namespace animation
	{
		class mixer
		{
		public:
			~mixer();
			mixer(engine::graphics::renderer & renderer, engine::physics::simulation & simulation);
		};

		void update(mixer & mixer);

		struct TransformSource
		{
			struct Action
			{
				engine::Asset name;
				core::container::Buffer positionx;
				core::container::Buffer positiony;
				core::container::Buffer positionz;
				core::container::Buffer rotationx;
				core::container::Buffer rotationy;
				core::container::Buffer rotationz;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("name"), &Action::name),
						std::make_pair(utility::string_units_utf8("position x"), &Action::positionx),
						std::make_pair(utility::string_units_utf8("position y"), &Action::positiony),
						std::make_pair(utility::string_units_utf8("position z"), &Action::positionz),
						std::make_pair(utility::string_units_utf8("rotation x"), &Action::rotationx),
						std::make_pair(utility::string_units_utf8("rotation y"), &Action::rotationy),
						std::make_pair(utility::string_units_utf8("rotation z"), &Action::rotationz)
					);
				}
			};

			std::vector<Action> actions;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("object actions"), &TransformSource::actions)
				);
			}
		};

		bool create_transform_source(mixer & mixer, engine::MutableEntity source, TransformSource && data);
		bool destroy_source(mixer & mixer, engine::MutableEntity source);

		bool attach_playback_animation(mixer & mixer, engine::Entity animation, engine::Entity source);
		bool detach_animation(mixer & mixer, engine::Entity animation);

		bool set_animation_action(mixer & mixer, engine::Entity animation, engine::Asset action);
		//bool set_animation_frame(mixer & mixer, engine::Entity animation, float frame); // todo
		//bool set_animation_speed(mixer & mixer, engine::Entity animation, float speed); // todo

		bool add_object(mixer & mixer, engine::Entity object, engine::Entity animation);
		bool remove_object(mixer & mixer, engine::Entity object);
	}
}
