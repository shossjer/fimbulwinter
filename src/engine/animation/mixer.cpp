
#include "mixer.hpp"

#include "Armature.hpp"
#include "Callbacks.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/Entity.hpp>
#include <engine/extras/Asset.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>

#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void update(engine::Entity entity, CharacterSkinning data);
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
		engine::extras::Asset,
		201,
		std::array<Armature, 101>,
		std::array<int, 1> // because
	>
	resources;

	struct Fade
	{
		float elapsed;
		float stepsize;

		Mixer from;
		Mixer to;

		Action::Frame compute()
		{

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
		core::maths::Vector3f movement;

		Playback(const Armature & armature, const bool repeat) : armature(&armature), action(nullptr), repeat(repeat), finished(false), framei(0), matrices(armature.njoints), movement(0.f, 0.f, 0.f) {}
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

				framei %= action->length;// framei = 0;
			}

			int rooti = 0;
			while (rooti < static_cast<int>(armature->njoints))
				rooti = update(*action, rooti, core::maths::Matrix4x4f::identity(), framei);
				// rooti = update(*action, rooti, /*core::maths::Matrix4x4f::identity(), */framei);
			
			if (!action->positions.empty())
			{
			// 	debug_printline(0xffffffff, "running speed is: ", length(action->positions[framei + 1] - action->positions[framei]));
				// movement = action->positions[framei];
				movement = action->positions[framei + 1] - action->positions[framei];
			}
		}
		int update(const Action & action, const int mei, const core::maths::Matrix4x4f & parent_matrix, const int framei)
		// int update(const Action & action, const int mei, /*const core::maths::Matrix4x4f & parent_matrix, */const int framei)
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
				// childi = update(action, childi, /*matrices[mei], */framei);
			return childi;
		}

		void extract(std::vector<core::maths::Matrix4x4f> & matrix_pallet) const
		{
			// vvvvvvvv tmp
			debug_assert(matrix_pallet.size() == matrices.size());
			for (int i = 0; i < static_cast<int>(armature->njoints); i++)
				matrix_pallet[i] = matrices[i] * armature->joints[i].inv_matrix;
			// ^^^"^^^^ tmp
		}
		void extract(core::maths::Vector3f & movement) const
		{
			movement = this->movement;
		}
	};

	core::container::Collection
	<
		Mixer,
		1001,
		std::array<Fade, 101>,
		std::array<Fadein, 101>,
		std::array<Fadeout, 101>,
		std::array<Playback, 101>
	>
	mixers;

	Mixer next_mixer_key = 0;

	struct extract_movement
	{
		core::maths::Vector3f & movement;

		extract_movement(core::maths::Vector3f & movement) : movement(movement) {}

		void operator () (Playback & x)
		{
			x.extract(movement);
		}
		template <typename X>
		void operator () (X & x)
		{
			debug_unreachable();
		}
	};
	struct extract_pallet
	{
		std::vector<core::maths::Matrix4x4f> & matrix_pallet;

		extract_pallet(std::vector<core::maths::Matrix4x4f> & matrix_pallet) : matrix_pallet(matrix_pallet) {}

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
	struct is_finished
	{
		bool operator () (Playback & x)
		{
			return x.isFinished();
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
		core::maths::Vector3f movement;

		Character(engine::Entity me, const Armature & armature) :
			me(me),
			armature(&armature),
			mixer(-1),
			matrix_pallet(armature.njoints)
		{
		}

		void finalize()
		{
			if (this->mixer == Mixer(-1))
				return;
			
			if (mixers.call(mixer, is_finished{}))
			{
				pCallbacks->onFinish(this->me);
			}

			mixers.call(mixer, extract_pallet{matrix_pallet});
			mixers.call(mixer, extract_movement{movement});
			engine::graphics::renderer::update(me,
			                                   engine::graphics::renderer::CharacterSkinning{matrix_pallet});
			engine::physics::post_update_movement(me, movement);
		}
	};

	core::container::Collection
	<
		engine::Entity,
		201,
		std::array<Character, 101>,
		std::array<int, 1> // because
	>
	components;

	struct get_armature
	{
		const Armature & operator () (const Character & x)
		{
			return *x.armature;
		}
		template <typename X>
		const Armature & operator () (const X & x)
		{
			debug_unreachable();
		}
	};
	struct set_action
	{
		Mixer mixer;

		set_action(Mixer mixer) : mixer(mixer) {}
		
		void operator () (Character & x)
		{
			if (x.mixer != Mixer(-1))
				mixers.remove(x.mixer);
			
			x.mixer = mixer;
		}
		template <typename X>
		void operator () (X & x)
		{
			debug_unreachable();
		}
	};

	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::animation::armature>,
	                                 20> queue_add_armature;
	core::container::CircleQueueSRMW<engine::Entity,
	                                 20> queue_remove;
	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::animation::action>,
	                                 50> queue_update_action;
}

namespace engine
{
	namespace animation
	{
		/**
		 * Sets callback instance, called from looper during setup
		 */
		void initialize(const Callbacks & callbacks)
		{
			pCallbacks = &callbacks;
		}

		void update()
		{
			// receive messages
			std::pair<engine::Entity, armature> message_add_armature;
			while (queue_add_armature.try_pop(message_add_armature))
			{
				// TODO: this should be done in a loader thread somehow
				const engine::extras::Asset armasset{message_add_armature.second.armfile};
				if (!resources.contains(armasset))
				{
					std::ifstream file(message_add_armature.second.armfile, std::ifstream::binary | std::ifstream::in);
					Armature arm;
					arm.read(file);
					resources.add(armasset, std::move(arm));
				}
				components.emplace<Character>(message_add_armature.first, message_add_armature.first, resources.get<Armature>(armasset));
			}
			engine::Entity message_remove;
			while (queue_remove.try_pop(message_remove))
			{
				// TODO: remove assets that no one uses any more
				components.remove(message_remove);
			}
			std::pair<engine::Entity, action> message_update_action;
			while (queue_update_action.try_pop(message_update_action))
			{
				// components.update(message_update_action.first, Character::SetAction{message_update_action.second.name});

				// THIS IS SUPER UGLY
				// YUCK!
				const auto & armature = components.call(message_update_action.first, get_armature{});
				const Mixer mixer = next_mixer_key++;
				auto & playback = mixers.emplace<Playback>(mixer, armature, message_update_action.second.repetative);
				{
					auto action = std::find(armature.actions.begin(),
					                        armature.actions.end(),
					                        message_update_action.second.name);
					if (action == armature.actions.end())
					{
						debug_printline(0xffffffff, "Could not find action ", message_update_action.second.name, " in armature ", armature.name);
					}
					else
					{
						playback.action = &*action;
					}
				}

				components.call(message_update_action.first, set_action{mixer});
			}

			// update stuff
			for (auto && playback : mixers.get<Playback>())
			{
				playback.update();
			}

			// send messages
			for (auto && character : components.get<Character>())
			{
				character.finalize();
			}
		}

		void add(engine::Entity entity, const armature & data)
		{
			const auto res = queue_add_armature.try_emplace(entity, data);
			debug_assert(res);
		}
		void remove(engine::Entity entity)
		{
			const auto res = queue_remove.try_push(entity);
			debug_assert(res);
		}
		void update(engine::Entity entity, const action & data)
		{
			const auto res = queue_update_action.try_emplace(entity, data);
			debug_assert(res);
		}
	}
}
