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

#include "utility/predicates.hpp"
#include "utility/profiling.hpp"
#include "utility/variant.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace
{
	engine::graphics::renderer * renderer = nullptr;
	engine::physics::simulation * simulation = nullptr;

	using Action = engine::animation::Armature::Action;
	using Armature = engine::animation::Armature;
	using Joint = engine::animation::Armature::Joint;
	using Mixer = unsigned int;

	constexpr auto invalid_mixer = Mixer(-1);

	const engine::animation::Callbacks * pCallbacks;

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<201>,
		utility::static_storage<101, Armature>,
		utility::static_storage<101, engine::animation::object>
	>
	sources;

	struct Fade
	{
		float elapsed;
		float stepsize;

		Mixer from;
		Mixer to;

		Action::Frame compute()
		{
			debug_unreachable();
		}
	};
	struct Fadein
	{
		Mixer to;
	};
	struct Fadeout
	{
		Mixer from;
	};
	struct Playback
	{
		const Armature * armature;
		const Action * action_;

		float elapsed;
		float stepsize;
		float length_;
		int times_to_play;

		bool repeat;
		bool finished;
		int framei_;

		// vvvvvvvv tmp
		std::vector<core::maths::Matrix4x4f> matrices;
		core::maths::Vector3f position_movement;
		core::maths::Quaternionf orientation_movement;

		Playback(const Armature & armature, bool repeat)
			: armature(&armature)
			, action_(nullptr)
			, repeat(repeat)
			, finished(false)
			, framei_(0)
			, matrices(armature.joints.size())
		{}
		// ^^^"^^^^ tmp

		bool isFinished() const
		{
			return finished;
		}

		void update()
		{
			if (action_ == nullptr)
				return;

			framei_ += 1;
			if (framei_ >= action_->length)
			{
				if (!repeat)
				{
					finished = true;
				}

				framei_ %= action_->length;
			}

			int rooti = 0;
			while (rooti < static_cast<int>(armature->joints.size()))
				rooti = update(*action_, rooti, core::maths::Matrix4x4f::identity(), framei_);

			if (!action_->positions.empty())
			{
				position_movement = action_->positions[framei_ + 1] - action_->positions[framei_];
			}
			if (!action_->orientations.empty())
			{
				orientation_movement = inverse(action_->orientations[framei_]) * action_->orientations[framei_ + 1];
			}
		}

		int update(const Action & action, int mei, const core::maths::Matrix4x4f & parent_matrix, int framei)
		{
			const auto pos = action.frames[framei].channels[mei].translation;
			const auto rot = action.frames[framei].channels[mei].rotation;
			const auto scale = action.frames[framei].channels[mei].scale;

			// vvvvvvvv tmp
			const auto pose =
				make_translation_matrix(pos) *
				make_matrix(rot) *
				make_scale_matrix(scale);

			matrices[mei] = parent_matrix * pose;
			// ^^^"^^^^ tmp

			int childi = mei + 1;
			for (int i = 0; i < static_cast<int>(armature->joints[mei].nchildren); i++)
				childi = update(action, childi, matrices[mei], framei);
			return childi;
		}

		void extract(std::vector<core::maths::Matrix4x4f> & matrix_pallet) const
		{
			// vvvvvvvv tmp
			debug_assert(matrix_pallet.size() == matrices.size());
			for (int i = 0; i < static_cast<int>(armature->joints.size()); i++)
				matrix_pallet[i] = matrices[i] * armature->joints[i].inv_matrix;
			// ^^^"^^^^ tmp
		}

		bool extract_position_movement(core::maths::Vector3f & movement) const
		{
			if (!action_->positions.empty())
			{
				movement = position_movement;
				return true;
			}
			return false;
		}

		bool extract_orientation_movement(core::maths::Quaternionf & movement) const
		{
			if (!action_->orientations.empty())
			{
				movement = orientation_movement;
				return true;
			}
			return false;
		}
	};

	struct ObjectPlayback
	{
		const engine::animation::object * object;
		const engine::animation::object::action * action;

		bool repeat;
		bool finished;
		int framei;

		ObjectPlayback(const engine::animation::object & object, bool repeat) :
			object(& object),
			action(nullptr),
			repeat(repeat),
			finished(false),
			framei(0) {}

		void update()
		{
			debug_assert(!finished);
			if (action == nullptr)
				return;

			// const int framei_prev = framei;
			const int framei_next = framei + 1;

			// movement = action->keys[framei_next].translation - action->keys[framei_prev].translation;

			debug_assert(!action->keys.empty());
			const int action_length = debug_cast<int>(action->keys.size()) - 1;
			if (framei_next < action_length)
			{
				framei = framei_next;
			}
			else if (repeat)
			{
				framei = framei_next % action_length;
			}
			else
			{
				finished = true;
				framei = action_length;
			}
		}

		engine::transform_t extract_translation() const
		{
			const auto & val = action->keys[framei];
			return engine::transform_t {val.translation, val.rotation};
		}
	};

	core::container::Collection
	<
		Mixer,
		utility::heap_storage_traits,
		utility::heap_storage<Fade>,
		utility::heap_storage<Fadein>,
		utility::heap_storage<Fadeout>,
		utility::heap_storage<Playback>,
		utility::heap_storage<ObjectPlayback>
	>
	mixers;

	Mixer next_mixer_key = 0;

	struct extract_position_movement
	{
		core::maths::Vector3f & movement;

		bool operator () (Playback & x)
		{
			return x.extract_position_movement(movement);
		}
		template <typename X>
		bool operator () (X &)
		{
			debug_unreachable();
		}
	};
	struct extract_orientation_movement
	{
		core::maths::Quaternionf & movement;

		bool operator () (Playback & x)
		{
			return x.extract_orientation_movement(movement);
		}
		template <typename X>
		bool operator () (X &)
		{
			debug_unreachable();
		}
	};
	struct extract_pallet
	{
		std::vector<core::maths::Matrix4x4f> & matrix_pallet;

		void operator () (Playback & x)
		{
			x.extract(matrix_pallet);
		}
		template <typename X>
		void operator () (X &)
		{
			debug_unreachable();
		}
	};
	struct extract_translation
	{
		engine::transform_t operator () (const ObjectPlayback & x)
		{
			return x.extract_translation();
		}
		template <typename X>
		engine::transform_t operator () (const X &)
		{
			debug_unreachable();
		}
	};
	struct is_finished
	{
		bool operator () (Playback & x)
		{
			return x.isFinished();
		}
		bool operator () (ObjectPlayback & x)
		{
			return x.finished;
		}
		template <typename X>
		bool operator () (X &)
		{
			debug_unreachable();
		}
	};

	struct Character
	{
		engine::Token me;

		const Armature * armature;

		Mixer mixer;

		std::vector<core::maths::Matrix4x4f> matrix_pallet;
		core::maths::Vector3f position_movement;
		core::maths::Quaternionf orientation_movement;

		Character(engine::Token me, const Armature & armature)
			: me(me)
			, armature(&armature)
			, mixer(invalid_mixer)
			, matrix_pallet(armature.joints.size())
		{}

		void finalize()
		{
			if (mixer == invalid_mixer)
				return;

			const auto mixer_it = find(mixers, mixer);

			if (mixers.call(mixer_it, is_finished{}))
			{
				pCallbacks->onFinish(this->me);
			}

			mixers.call(mixer_it, extract_pallet{matrix_pallet});
			const bool has_position_movement = mixers.call(mixer_it, extract_position_movement{position_movement});
			const bool has_orientation_movement = mixers.call(mixer_it, extract_orientation_movement{orientation_movement});
			post_update_characterskinning(
				*renderer,
				me,
				engine::graphics::data::CharacterSkinning{matrix_pallet});
			if (has_position_movement)
			{
				post_update_movement(*::simulation, me, engine::physics::movement_data{engine::physics::movement_data::Type::CHARACTER, position_movement});
			}
			if (has_orientation_movement)
			{
				post_update_orientation_movement(*::simulation, me, engine::physics::orientation_movement{orientation_movement});
			}
		}
	};
	struct Model
	{
		engine::Token me;

		const engine::animation::object * object;

		Mixer mixer;

		Model(engine::Token me, const engine::animation::object & object)
			: me(me)
			, object(&object)
			, mixer(invalid_mixer)
		{}

		void finalize()
		{
			if (mixer == invalid_mixer)
				return;

			const auto mixer_it = find(mixers, mixer);

			post_update_movement(*::simulation, me, mixers.call(mixer_it, extract_translation{}));

			if (mixers.call(mixer_it, is_finished{}))
			{
				//pCallbacks->onFinish(this->me);
				// TODO: this needs to be changed when we start animation blending
				mixers.erase(mixer_it);
				mixer = invalid_mixer;
			}
		}
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<Character>,
		utility::heap_storage<Model>
	>
	components;

	struct set_action
	{
		engine::animation::action data;

		void operator () (Character & x)
		{
			const Mixer mixer = next_mixer_key++;
			auto * const playback = mixers.emplace<Playback>(mixer, *x.armature, data.repetative);
			if (!debug_verify(playback))
				return; // error
			{
				auto action = std::find(x.armature->actions.begin(),
				                        x.armature->actions.end(),
				                        utility::if_name_is(data.name));
				if (action == x.armature->actions.end())
				{
					debug_printline(engine::animation_channel, "Could not find action ", data.name, " in armature ", x.armature->name);
				}
				else
				{
					playback->action_ = &*action;
				}
			}
			// set mixer
			if (x.mixer != invalid_mixer)
				mixers.erase(find(mixers, x.mixer));
			x.mixer = mixer;
		}
		void operator () (Model & x)
		{
			const Mixer mixer = next_mixer_key++;
			auto * const objectplayback = mixers.emplace<ObjectPlayback>(mixer, *x.object, data.repetative);
			if (!debug_verify(objectplayback))
				return; // error
			{
				auto action = std::find(x.object->actions.begin(),
				                        x.object->actions.end(),
				                        utility::if_name_is(data.name));
				if (action == x.object->actions.end())
				{
					debug_printline(engine::animation_channel, "Could not find action ", data.name, " in object ", x.object->name);
				}
				else
				{
					objectplayback->action = &*action;
				}
			}
			// set mixer
			if (x.mixer != invalid_mixer)
				mixers.erase(find(mixers, x.mixer));
			x.mixer = mixer;
		}
	};

	struct MessageRegisterArmature
	{
		engine::Token asset;
		engine::animation::Armature data;
	};
	struct MessageRegisterObject
	{
		engine::Token asset;
		engine::animation::object data;
	};
	using AssetMessage = utility::variant
	<
		MessageRegisterArmature,
		MessageRegisterObject
	>;

	struct MessageAddCharacter
	{
		engine::Token entity;
		engine::animation::character data;
	};
	struct MessageAddModel
	{
		engine::Token entity;
		engine::animation::model data;
	};
	struct MessageUpdateAction
	{
		engine::Token entity;
		engine::animation::action data;
	};
	struct MessageRemove
	{
		engine::Token entity;
	};
	using EntityMessage = utility::variant
	<
		MessageAddCharacter,
		MessageAddModel,
		MessageUpdateAction,
		MessageRemove
	>;

	core::container::PageQueue<utility::heap_storage<AssetMessage>> queue_assets;
	core::container::PageQueue<utility::heap_storage<EntityMessage>> queue_entities;
}

namespace engine
{
	namespace animation
	{
		mixer::~mixer()
		{
			engine::Token sources_not_unregistered[sources.max_size()];
			const auto source_count = sources.get_all_keys(sources_not_unregistered, sources.max_size());
			debug_printline(source_count, " sources not unregistered:");
			for (auto i : ranges::index_sequence(source_count))
			{
				debug_printline(sources_not_unregistered[i]);
				static_cast<void>(i);
			}

			::simulation = nullptr;
			::renderer = nullptr;
		}

		mixer::mixer(engine::graphics::renderer & renderer_, engine::physics::simulation & simulation_)
		{
			::renderer = &renderer_;
			::simulation = &simulation_;
		}

		/**
		 * Sets callback instance, called from looper during setup
		 */
		void initialize(mixer &, const Callbacks & callbacks)
		{
			pCallbacks = &callbacks;
		}

		void update(mixer &)
		{
			profile_scope("mixer update");

			AssetMessage asset_message;
			while (queue_assets.try_pop(asset_message))
			{
				struct ProcessMessage
				{
					void operator () (MessageRegisterArmature && x)
					{
						static_cast<void>(debug_verify(sources.emplace<Armature>(x.asset, std::move(x.data))));
					}
					void operator () (MessageRegisterObject && x)
					{
						static_cast<void>(debug_verify(sources.emplace<engine::animation::object>(x.asset, std::move(x.data))));
					}
				};
				visit(ProcessMessage{}, std::move(asset_message));
			}

			EntityMessage entity_message;
			while (queue_entities.try_pop(entity_message))
			{
				struct ProcessMessage
				{
					void operator () (MessageAddCharacter && x)
					{
						const auto source_it = find(sources, x.data.armature);
						if (!debug_assert(source_it != sources.end()))
							return; // error

						const Armature * const armature = sources.get<Armature>(source_it);
						if (!debug_assert(armature))
							return; // error

						static_cast<void>(debug_verify(components.emplace<Character>(x.entity, x.entity, *armature)));
					}
					void operator () (MessageAddModel && x)
					{
						const auto source_it = find(sources, x.data.object);
						if (!debug_assert(source_it != sources.end()))
							return; // error

						const engine::animation::object * const object = sources.get<engine::animation::object>(source_it);
						if (!debug_assert(object))
							return; // error

						static_cast<void>(debug_verify(components.emplace<Model>(x.entity, x.entity, *object)));
					}
					void operator () (MessageUpdateAction && x)
					{
						const auto component_it = find(components, x.entity);
						if (debug_assert(component_it != components.end()))
						{
							components.call(component_it, set_action{std::move(x.data)});
						}
					}
					void operator () (MessageRemove && x)
					{
						const auto component_it = find(components, x.entity);
						if (debug_assert(component_it != components.end()))
						{
							components.erase(component_it);
						}
					}
				};
				visit(ProcessMessage{}, std::move(entity_message));
			}

			// update stuff
			for (auto & playback : mixers.get<Playback>())
			{
				playback.update();
			}
			for (auto & objectplayback : mixers.get<ObjectPlayback>())
			{
				objectplayback.update();
			}

			// send messages
			for (auto & character : components.get<Character>())
			{
				character.finalize();
			}
			for (auto & model : components.get<Model>())
			{
				model.finalize();
			}
		}

		void post_register_armature(mixer &, engine::Token asset, engine::animation::Armature && data)
		{
			debug_verify(queue_assets.try_emplace(utility::in_place_type<MessageRegisterArmature>, asset, std::move(data)));
		}

		void post_register_object(mixer &, engine::Token asset, engine::animation::object && data)
		{
			debug_verify(queue_assets.try_emplace(utility::in_place_type<MessageRegisterObject>, asset, std::move(data)));
		}

		void post_add_character(mixer &, engine::Token entity, engine::animation::character && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddCharacter>, entity, std::move(data)));
		}

		void post_add_model(mixer &, engine::Token entity, engine::animation::model && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddModel>, entity, std::move(data)));
		}

		void post_update_action(mixer &, engine::Token entity, engine::animation::action && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateAction>, entity, std::move(data)));
		}

		void post_remove(mixer &, engine::Token entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity));
		}
	}
}
