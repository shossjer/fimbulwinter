
#include "mixer.hpp"

#include "Armature.hpp"
#include "Callbacks.hpp"

#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"
#include "engine/Entity.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/physics/physics.hpp"
#include "engine/resource/reader.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void post_update_characterskinning(engine::Entity entity, CharacterSkinning && data);
		}
	}
}

namespace
{
	using Action = engine::animation::Armature::Action;
	using Armature = engine::animation::Armature;
	using Joint = engine::animation::Armature::Joint;
	using Mixer = unsigned int;

	const engine::animation::Callbacks * pCallbacks;

	core::container::UnorderedCollection
	<
		engine::Asset,
		201,
		std::array<Armature, 101>,
		std::array<engine::animation::object, 101>
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
		const Action * action;

		float elapsed;
		float stepsize;
		float length_;
		int times_to_play;

		bool repeat;
		bool finished;
		int framei;

		// vvvvvvvv tmp
		std::vector<core::maths::Matrix4x4f> matrices;
		core::maths::Vector3f position_movement;
		core::maths::Quaternionf orientation_movement;

		Playback(const Armature & armature, const bool repeat) : armature(&armature), action(nullptr), repeat(repeat), finished(false), framei(0), matrices(armature.joints.size()) {}
		// ^^^"^^^^ tmp

		bool isFinished() const
		{
			return finished;
		}

		void update()
		{
			if (action == nullptr)
				return;

			framei += 1;
			if (framei >= action->length)
			{
				if (!repeat)
				{
					finished = true;
				}

				framei %= action->length;
			}

			int rooti = 0;
			while (rooti < static_cast<int>(armature->joints.size()))
				rooti = update(*action, rooti, core::maths::Matrix4x4f::identity(), framei);

			if (!action->positions.empty())
			{
				position_movement = action->positions[framei + 1] - action->positions[framei];
			}
			if (!action->orientations.empty())
			{
				orientation_movement = inverse(action->orientations[framei]) * action->orientations[framei + 1];
			}
		}
		int update(const Action & action, const int mei, const core::maths::Matrix4x4f & parent_matrix, const int framei)
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
			if (!action->positions.empty())
			{
				movement = position_movement;
				return true;
			}
			return false;
		}
		bool extract_orientation_movement(core::maths::Quaternionf & movement) const
		{
			if (!action->orientations.empty())
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

			const auto framei_prev = framei;
			const auto framei_next = framei + 1;

			// movement = action->keys[framei_next].translation - action->keys[framei_prev].translation;

			const auto action_length = action->keys.size() - 1;
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
		1001,
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
		bool operator () (X & x)
		{
			debug_unreachable();
			return false;
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
		bool operator () (X & x)
		{
			debug_unreachable();
			return false;
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
		void operator () (X & x)
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
		engine::transform_t operator () (const X & x)
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
		bool operator () (X & x)
		{
			debug_unreachable();
		}
	};

	struct Character
	{
		engine::Entity me;

		const Armature * armature;

		Mixer mixer;

		std::vector<core::maths::Matrix4x4f> matrix_pallet;
		core::maths::Vector3f position_movement;
		core::maths::Quaternionf orientation_movement;

		Character(engine::Entity me, const Armature & armature) :
			me(me),
			armature(&armature),
			mixer(-1),
			matrix_pallet(armature.joints.size())
		{}

		void finalize()
		{
			if (this->mixer == Mixer(-1))
				return;

			if (mixers.call(mixer, is_finished{}))
			{
				pCallbacks->onFinish(this->me);
			}

			mixers.call(mixer, extract_pallet{matrix_pallet});
			const bool has_position_movement = mixers.call(mixer, extract_position_movement{position_movement});
			const bool has_orientation_movement = mixers.call(mixer, extract_orientation_movement{orientation_movement});
			engine::graphics::renderer::post_update_characterskinning(me,
			                                   engine::graphics::renderer::CharacterSkinning{matrix_pallet});
			if (has_position_movement)
			{
				engine::physics::post_update_movement(me, engine::physics::movement_data{engine::physics::movement_data::Type::CHARACTER, position_movement});
			}
			if (has_orientation_movement)
			{
				engine::physics::post_update_orientation_movement(me, engine::physics::orientation_movement{orientation_movement});
			}
		}
	};
	struct Model
	{
		engine::Entity me;

		const engine::animation::object * object;

		Mixer mixer;

		Model(engine::Entity me, const engine::animation::object & object) :
			me(me),
			object(& object),
			mixer(-1)
		{
		}

		void finalize()
		{
			if (this->mixer == Mixer(-1))
				return;

			engine::physics::post_update_movement(me, mixers.call(mixer, extract_translation {}));

			if (mixers.call(mixer, is_finished{}))
			{
				//pCallbacks->onFinish(this->me);
				// TODO: this needs to be changed when we start animation blending
				mixers.remove(mixer);
				mixer = Mixer(-1);
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		201,
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
			auto & playback = mixers.emplace<Playback>(mixer, *x.armature, data.repetative);
			{
				auto action = std::find(x.armature->actions.begin(),
				                        x.armature->actions.end(),
				                        data.name);
				if (action == x.armature->actions.end())
				{
					debug_printline(engine::animation_channel, "Could not find action ", data.name, " in armature ", x.armature->name);
				}
				else
				{
					playback.action = &*action;
				}
			}
			// set mixer
			if (x.mixer != Mixer(-1))
				mixers.remove(x.mixer);
			x.mixer = mixer;
		}
		void operator () (Model & x)
		{
			const Mixer mixer = next_mixer_key++;
			auto & objectplayback = mixers.emplace<ObjectPlayback>(mixer, *x.object, data.repetative);
			{
				auto action = std::find(x.object->actions.begin(),
				                        x.object->actions.end(),
				                        data.name);
				if (action == x.object->actions.end())
				{
					debug_printline(engine::animation_channel, "Could not find action ", data.name, " in object ", x.object->name);
				}
				else
				{
					objectplayback.action = &*action;
				}
			}
			// set mixer
			if (x.mixer != Mixer(-1))
				mixers.remove(x.mixer);
			x.mixer = mixer;
		}
	};

	struct MessageRegisterArmature
	{
		engine::Asset asset;
		engine::animation::Armature data;
	};
	struct MessageRegisterObject
	{
		engine::Asset asset;
		engine::animation::object data;
	};
	using AssetMessage = utility::variant
	<
		MessageRegisterArmature,
		MessageRegisterObject
	>;

	struct MessageAddCharacter
	{
		engine::Entity entity;
		engine::animation::character data;
	};
	struct MessageAddModel
	{
		engine::Entity entity;
		engine::animation::model data;
	};
	struct MessageUpdateAction
	{
		engine::Entity entity;
		engine::animation::action data;
	};
	struct MessageRemove
	{
		engine::Entity entity;
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
		void create()
		{}

		void destroy()
		{
			engine::Asset sources_not_unregistered[sources.max_size()];
			const int source_count = sources.get_all_keys(sources_not_unregistered, sources.max_size());
			debug_printline(source_count, " sources not unregistered:");
			for (int i = 0; i < source_count; i++)
			{
				debug_printline(sources_not_unregistered[i]);
			}
		}

		/**
		 * Sets callback instance, called from looper during setup
		 */
		void initialize(const Callbacks & callbacks)
		{
			pCallbacks = &callbacks;
		}

		void update()
		{
			AssetMessage asset_message;
			while (queue_assets.try_pop(asset_message))
			{
				struct ProcessMessage
				{
					void operator () (MessageRegisterArmature && x)
					{
						debug_assert(!sources.contains(x.asset));
						sources.emplace<Armature>(x.asset, std::move(x.data));
					}
					void operator () (MessageRegisterObject && x)
					{
						debug_assert(!sources.contains(x.asset));
						sources.emplace<engine::animation::object>(x.asset, std::move(x.data));
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
						debug_assert(sources.contains<Armature>(x.data.armature));
						const auto & armature = sources.get<Armature>(x.data.armature);

						components.emplace<Character>(x.entity, x.entity, armature);
					}
					void operator () (MessageAddModel && x)
					{
						debug_assert(sources.contains<engine::animation::object>(x.data.object));
						const auto & object = sources.get<engine::animation::object>(x.data.object);

						components.emplace<Model>(x.entity, x.entity, object);
					}
					void operator () (MessageUpdateAction && x)
					{
						components.call(x.entity, set_action{std::move(x.data)});
					}
					void operator () (MessageRemove && x)
					{
						components.remove(x.entity);
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

		void post_register_armature(engine::Asset asset, engine::animation::Armature && data)
		{
			const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterArmature>, asset, std::move(data));
			debug_assert(res);
		}

		void post_register_object(engine::Asset asset, engine::animation::object && data)
		{
			const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterObject>, asset, std::move(data));
			debug_assert(res);
		}

		void post_add_character(engine::Entity entity, engine::animation::character && data)
		{
			const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddCharacter>, entity, std::move(data));
			debug_assert(res);
		}

		void post_add_model(engine::Entity entity, engine::animation::model && data)
		{
			const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddModel>, entity, std::move(data));
			debug_assert(res);
		}

		void post_update_action(engine::Entity entity, engine::animation::action && data)
		{
			const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateAction>, entity, std::move(data));
			debug_assert(res);
		}

		void post_remove(engine::Entity entity)
		{
			const auto res = queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity);
			debug_assert(res);
		}
	}
}
