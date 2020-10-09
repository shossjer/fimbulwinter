#pragma once

#include "core/container/Buffer.hpp"
#include "core/serialization.hpp"

#include "engine/animation/Armature.hpp"
#include "engine/Asset.hpp"
#include "engine/common.hpp"
#include "engine/Token.hpp"

#include "utility/container/array.hpp"

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
					return utility::make_lookup_table<ful::view_utf8>(
						std::make_pair(ful::cstr_utf8("name"), &Action::name),
						std::make_pair(ful::cstr_utf8("position x"), &Action::positionx),
						std::make_pair(ful::cstr_utf8("position y"), &Action::positiony),
						std::make_pair(ful::cstr_utf8("position z"), &Action::positionz),
						std::make_pair(ful::cstr_utf8("rotation x"), &Action::rotationx),
						std::make_pair(ful::cstr_utf8("rotation y"), &Action::rotationy),
						std::make_pair(ful::cstr_utf8("rotation z"), &Action::rotationz)
					);
				}
			};

			utility::heap_array<Action> actions;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("object actions"), &TransformSource::actions)
				);
			}
		};

		bool create_transform_source(mixer & mixer, engine::Token source, TransformSource && data);
		bool destroy_source(mixer & mixer, engine::Token source);

		bool attach_playback_animation(mixer & mixer, engine::Token animation, engine::Token source);
		bool detach_animation(mixer & mixer, engine::Token animation);

		bool set_animation_action(mixer & mixer, engine::Token animation, engine::Asset action);
		//bool set_animation_frame(mixer & mixer, engine::Token animation, float frame); // todo
		//bool set_animation_speed(mixer & mixer, engine::Token animation, float speed); // todo

		bool add_object(mixer & mixer, engine::Token object, engine::Token animation);
		bool remove_object(mixer & mixer, engine::Token object);
	}
}
