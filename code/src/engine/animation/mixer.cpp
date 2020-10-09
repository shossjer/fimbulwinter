#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"

#include "engine/animation/Armature.hpp"
#include "engine/animation/Callbacks.hpp"
#include "engine/animation/mixer.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/physics/physics.hpp"
#include "engine/Token.hpp"

#include "utility/algorithm.hpp"
#include "utility/algorithm/find.hpp"
#include "utility/functional/utility.hpp"
#include "utility/predicates.hpp"
#include "utility/variant.hpp"

namespace
{
	engine::graphics::renderer * renderer = nullptr;
	engine::physics::simulation * simulation = nullptr;

	template <typename T>
	bool try_clamp_above(T & value, const T & hi)
	{
		if (value < hi)
			return false;

		value = hi;
		return true;
	}

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<201>,
		utility::static_storage<101, engine::animation::TransformSource>
	>
	sources;

	struct TransformUpdateResult
	{
		uint16_t mask;
		bool done;
		float values[9]; // todo
	};
	static_assert(sizeof(TransformUpdateResult) == sizeof(float[10]), "bool is probably messing with alignment");

	struct TransformPlaybackAnimation
	{
		const engine::animation::TransformSource & source;

		const engine::animation::TransformSource::Action * action;
		float frame;
		float speed;

		explicit TransformPlaybackAnimation(const engine::animation::TransformSource & source)
			: source(source)
			, action(nullptr)
			, frame(0.f)
			, speed(0.f)
		{}

		void update(utility::heap_vector<TransformUpdateResult> & results)
		{
			results.try_emplace_back();
			ext::back(results).mask = 0;
			ext::back(results).done = true;

			if (action == nullptr)
				return;

			const float length = static_cast<float>(std::max({
				action->positionx.size(),
				action->positiony.size(),
				action->positionz.size(),
				action->rotationx.size(),
				action->rotationy.size(),
				action->rotationz.size()
				}));
			if (!debug_assert(0 < length))
				return;

			ext::back(results).done = try_clamp_above(frame += speed, length - 1.f);

			if (0 < action->positionx.size())
			{
				ext::back(results).mask |= 0x0001;
				ext::back(results).values[0] = action->positionx.data_as<float>()[static_cast<int>(frame)];
			}

			if (0 < action->positiony.size())
			{
				ext::back(results).mask |= 0x0002;
				ext::back(results).values[1] = action->positiony.data_as<float>()[static_cast<int>(frame)];
			}

			if (0 < action->positionz.size())
			{
				ext::back(results).mask |= 0x0004;
				ext::back(results).values[2] = action->positionz.data_as<float>()[static_cast<int>(frame)];
			}

			if (0 < action->rotationx.size())
			{
				ext::back(results).mask |= 0x0008;
				ext::back(results).values[3] = action->rotationx.data_as<float>()[static_cast<int>(frame)];
			}

			if (0 < action->rotationy.size())
			{
				ext::back(results).mask |= 0x0010;
				ext::back(results).values[4] = action->rotationy.data_as<float>()[static_cast<int>(frame)];
			}

			if (0 < action->rotationz.size())
			{
				ext::back(results).mask |= 0x0020;
				ext::back(results).values[5] = action->rotationz.data_as<float>()[static_cast<int>(frame)];
			}
		}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<201>,
		utility::static_storage<101, TransformPlaybackAnimation>
	>
	animations;

	utility::heap_vector<engine::Token> step_animation(engine::Token animation)
	{
		utility::heap_vector<engine::Token> order;

		utility::heap_vector<engine::Token, float> stack;
		stack.try_emplace_back(animation, 1.f);
		while (!ext::empty(stack))
		{
			const auto current = ext::back(stack);
			ext::pop_back(stack);

			order.push_back(current.first);

			struct
			{
				float scale;

				utility::heap_vector<engine::Token, float> & stack;

				void operator () (TransformPlaybackAnimation & x)
				{
					if (x.action == nullptr)
						return;

					const auto unit = x.speed * scale;
					x.frame += unit;
				}
			}
			visit_children_in_reverse{current.second, stack};

			const auto handle = find(animations, current.first);
			debug_assert(handle != animations.end());
			animations.call(handle, visit_children_in_reverse);
		}

		std::reverse(order.begin(), order.end());

		return order;
	}

	struct Object
	{
		engine::Token animation;

		explicit Object(engine::Token animation)
			: animation(animation)
		{
		}
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<Object>
	>
	objects;

	TransformUpdateResult update_animation(engine::Token animation)
	{
		auto order = step_animation(animation);

		utility::heap_vector<TransformUpdateResult> results;
		for (auto i : ranges::index_sequence_for(order))
		{
			const auto handle = find(animations, order[i]);
			debug_assert(handle != animations.end());
			animations.call(handle, [&](auto & x){ x.update(results); });
		}
		debug_assert(results.size() == 1);

		return ext::front(results);
	}

	struct MessageCreateTransformSource
	{
		engine::Token source;
		engine::animation::TransformSource data;
	};

	struct MessageDestroySource
	{
		engine::Token source;
	};

	struct MessageAttachPlaybackAnimation
	{
		engine::Token animation;
		engine::Token source;
	};

	struct MessageDetachAnimation
	{
		engine::Token animation;
	};

	struct MessageAddObject
	{
		engine::Token object;
		engine::Token animation;
	};

	struct MessageRemoveObject
	{
		engine::Token object;
	};

	struct MessageSetAnimationAction
	{
		engine::Token animation;
		engine::Asset action;
	};

	using Message = utility::variant
	<
		MessageCreateTransformSource,
		MessageDestroySource,
		MessageAttachPlaybackAnimation,
		MessageDetachAnimation,
		MessageAddObject,
		MessageRemoveObject,
		MessageSetAnimationAction
	>;

	core::container::PageQueue<utility::heap_storage<Message>> message_queue;
}

namespace engine
{
	namespace animation
	{
		mixer::~mixer()
		{
			engine::Token animations_not_unregistered[animations.max_size()];
			const auto animation_count = animations.get_all_keys(animations_not_unregistered, animations.max_size());
			debug_printline(animation_count, " animations not detached:");
			for (auto i : ranges::index_sequence(animation_count))
			{
				debug_printline(animations_not_unregistered[i]);
			}

			engine::Token sources_not_unregistered[sources.max_size()];
			const auto source_count = sources.get_all_keys(sources_not_unregistered, sources.max_size());
			debug_printline(source_count, " sources not unregistered:");
			for (auto i : ranges::index_sequence(source_count))
			{
				debug_printline(sources_not_unregistered[i]);
				fiw_unused(i);
			}

			::simulation = nullptr;
			::renderer = nullptr;
		}

		mixer::mixer(engine::graphics::renderer & renderer_, engine::physics::simulation & simulation_)
		{
			::renderer = &renderer_;
			::simulation = &simulation_;
		}

		void update(mixer &)
		{
			Message message;
			while (message_queue.try_pop(message))
			{
				struct
				{
					void operator () (MessageCreateTransformSource && x)
					{
						const auto handle = find(sources, x.source);
						if (handle != sources.end())
						{
							sources.erase(handle);
						}
						debug_verify(sources.emplace<TransformSource>(x.source, std::move(x.data)));
					}

					void operator () (MessageDestroySource && x)
					{
						const auto handle = find(sources, x.source);
						if (debug_verify(handle != sources.end()))
						{
							sources.erase(handle);
						}
					}

					void operator () (MessageAttachPlaybackAnimation && x)
					{
						const auto handle = find(sources, x.source);
						if (!debug_verify(handle != sources.end()))
							return; // error

						struct
						{
							engine::Token animation;

							void operator () (const TransformSource & x)
							{
								debug_verify(animations.emplace<TransformPlaybackAnimation>(animation, x));
							}
						}
						construct_animation{x.animation};

						sources.call(handle, construct_animation);
					}

					void operator () (MessageDetachAnimation && x)
					{
						const auto handle = find(animations, x.animation);
						if (debug_verify(handle != animations.end()))
						{
							animations.erase(handle);
						}
					}

					void operator () (MessageAddObject && x)
					{
						debug_verify(objects.emplace<Object>(x.object, x.animation));
					}

					void operator () (MessageRemoveObject && x)
					{
						const auto handle = find(objects, x.object);
						if (debug_verify(handle != objects.end()))
						{
							objects.erase(handle);
						}
					}

					void operator () (MessageSetAnimationAction && x)
					{
						const auto handle = find(animations, x.animation);
						if (!debug_verify(handle != animations.end()))
							return; // error

						struct
						{
							engine::Asset name;

							void operator () (TransformPlaybackAnimation & x)
							{
								auto maybe = std::find(x.source.actions.begin(), x.source.actions.end(), utility::if_name_is(name));
								if (!debug_verify(maybe != x.source.actions.end()))
									return;

								x.action = &*maybe;
								x.frame = 0.f; // todo
								x.speed = 1.f; // todo
							}
						}
						visitor{x.action};

						animations.call(handle, visitor);
					}
				}
				visitor;

				visit(visitor, std::move(message));
			}

			utility::heap_vector<engine::Token, TransformUpdateResult> active_animations;
			debug_verify(active_animations.try_reserve(objects.get<Object>().size()));
			for (auto & object : objects.get<Object>())
			{
				if (ext::find_if(active_animations, fun::first == object.animation) == active_animations.end())
				{
					active_animations.try_emplace_back(object.animation, update_animation(object.animation));
				}
			}

			for (auto & object : objects.get<Object>())
			{
				auto animation_it = ext::find_if(active_animations, fun::first == object.animation);
				debug_assert(animation_it != active_animations.end());

				if (!/*debug_verify*/((*animation_it).second.mask != 0, "an animation evaluated to nothing"))
					continue;

				const auto entity = objects.get_key(object);

				engine::physics::TransformComponents transform;
				transform.mask = (*animation_it).second.mask;
				for (auto i : ranges::index_sequence_for((*animation_it).second.values))
				{
					transform.values[i] = (*animation_it).second.values[i];
				}

				engine::physics::set_transform_components(*::simulation, entity, std::move(transform));
			}
		}

		bool create_transform_source(mixer & /*mixer*/, engine::Token source, TransformSource && data)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageCreateTransformSource>, source, std::move(data)));
		}

		bool destroy_source(mixer & /*mixer*/, engine::Token source)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageDestroySource>, source));
		}

		bool attach_playback_animation(mixer & /*mixer*/, engine::Token animation, engine::Token source)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAttachPlaybackAnimation>, animation, source));
		}

		bool detach_animation(mixer & /*mixer*/, engine::Token animation)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageDetachAnimation>, animation));
		}

		bool set_animation_action(mixer & /*mixer*/, engine::Token animation, engine::Asset action)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageSetAnimationAction>, animation, action));
		}

		bool add_object(mixer & /*mixer*/, engine::Token object, engine::Token animation)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAddObject>, object, animation));
		}

		bool remove_object(mixer & /*mixer*/, engine::Token object)
		{
			return debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRemoveObject>, object));
		}
	}
}
