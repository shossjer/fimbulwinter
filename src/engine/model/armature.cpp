
#include "armature.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>
#include <engine/extras/Asset.hpp>
#include <engine/graphics/renderer.hpp>

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
	void read_count(std::ifstream & stream, uint16_t & count)
	{
		stream.read(reinterpret_cast<char *>(& count), sizeof(uint16_t));
	}
	void read_matrix(std::ifstream & stream, core::maths::Matrix4x4f & matrix)
	{
		core::maths::Matrix4x4f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		matrix.set_aligned(buffer);
	}
	void read_quaternion(std::ifstream & stream, core::maths::Quaternionf & quat)
	{
		core::maths::Quaternionf::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		quat.set_aligned(buffer);
	}
	void read_vector(std::ifstream & stream, core::maths::Vector3f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set_aligned(buffer);
	}
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (&buffer)[N])
	{
		uint16_t len; // including null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
		debug_assert(len <= N);

		stream.read(buffer, len);
	}

	struct Joint
	{
		char name[16]; // arbitrary

		core::maths::Matrix4x4f matrix; // unused
		core::maths::Matrix4x4f inv_matrix;

		uint16_t parenti;
		uint16_t nchildren;
	};

	void read_joint_chain(std::ifstream & stream, std::vector<Joint> & joints, const int parenti)
	{
		const int mei = joints.size();
		joints.emplace_back();
		Joint & me = joints.back();

		read_string(stream, me.name);

		read_matrix(stream, me.matrix);
		read_matrix(stream, me.inv_matrix);

		me.parenti = parenti;

		read_count(stream, me.nchildren);
		for (int i = 0; i < static_cast<int>(me.nchildren); i++)
			read_joint_chain(stream, joints, mei);
	}

	struct Action
	{
		struct Frame
		{
			struct Channel
			{
				core::maths::Vector3f translation;
				core::maths::Quaternionf rotation;
				core::maths::Vector3f scale;
			};

			std::vector<Channel> channels;

			Frame(const int nchannels) :
				channels(nchannels)
			{}
		};

		char name[24]; // arbitrary

		int32_t length;

		std::vector<Frame> frames;
		std::vector<core::maths::Vector3f> positions;

		bool operator == (const std::string & name) const
		{
			return name == this->name;
		}
	};

	void read_action(std::ifstream & stream, std::vector<Action> & actions, const int njoints)
	{
		actions.emplace_back();
		Action & me = actions.back();

		read_string(stream, me.name);
		debug_printline(0xffffffff, me.name);

		stream.read(reinterpret_cast<char *>(& me.length), sizeof(int32_t));
		debug_printline(0xffffffff, me.length);

		me.frames.reserve(me.length + 1);
		for (int framei = 0; framei <= me.length; framei++)
		{
			me.frames.emplace_back(njoints);
			for (int i = 0; i < njoints; i++)
			{
				uint16_t index;
				read_count(stream, index);

				read_vector(stream, me.frames.back().channels[index].translation);
				read_quaternion(stream, me.frames.back().channels[index].rotation);
				read_vector(stream, me.frames.back().channels[index].scale);
			}
		}

		uint8_t has_positions;
		stream.read(reinterpret_cast<char *>(& has_positions), sizeof(uint8_t));
		debug_printline(0xffffffff, has_positions ? "has positions" : "has NOT positions");

		if (has_positions)
		{
			me.positions.resize(me.length + 1);
			for (int framei = 0; framei <= me.length; framei++)
			{
				read_vector(stream, me.positions[framei]);
			}
		}
	}

	struct Armature
	{
		char name[64]; // arbitrary

		uint16_t njoints;
		uint16_t nactions;

		std::vector<Joint> joints;
		std::vector<Action> actions;
	};

	void read_armature(std::ifstream & stream, Armature & armature)
	{
		read_string(stream, armature.name);
		debug_printline(0xffffffff, "armature name: ", armature.name);

		read_count(stream, armature.njoints);
		debug_printline(0xffffffff, "armature njoints: ", armature.njoints);

		uint16_t nroots;
		read_count(stream, nroots);
		debug_printline(0xffffffff, "armature nroots: ", nroots);

		armature.joints.reserve(armature.njoints);
		while (armature.joints.size() < armature.njoints)
			read_joint_chain(stream, armature.joints, -1);

		read_count(stream, armature.nactions);
		debug_printline(0xffffffff, "armature nactions: ", armature.nactions);

		armature.actions.reserve(armature.nactions);
		while (armature.actions.size() < armature.nactions)
			read_action(stream, armature.actions, armature.njoints);
	}

	struct Character
	{
		struct SetAction
		{
			std::string name;

			SetAction(std::string name) : name(name) {}
		};
		struct SetArmature
		{
			const Armature & armature;

			SetArmature(const Armature & armature) : armature(armature) {}
		};

		const Armature * armature;

		const Action * current_action;

		std::vector<core::maths::Matrix4x4f> matrices;
		std::vector<core::maths::Matrix4x4f> matrix_pallet;

		Character(SetArmature && data) :
			armature(&data.armature),
			current_action(nullptr),
			matrices(data.armature.njoints),
			matrix_pallet(data.armature.njoints)
		{}

		Character & operator = (const SetAction & data)
		{
			auto action = std::find(this->armature->actions.begin(),
			                        this->armature->actions.end(),
			                        data.name);
			if (action == this->armature->actions.end())
			{
				debug_printline(0xffffffff, "Could not find action ", data.name, " in armature ", armature->name);
				this->current_action = nullptr;
			}
			else
			{
				this->current_action = &*action;
			}
			return *this;
		}

		void update()
		{
			if (current_action == nullptr)
			{
				for (int i = 0; i < static_cast<int>(armature->njoints); i++)
					matrix_pallet[i] = core::maths::Matrix4x4f::identity();
				return;
			}
			const auto & action = *current_action;

			static int framei = 0;
			framei += 1;
			if (framei >= action.length) framei = 0;

			int rooti = 0;
			while (rooti < static_cast<int>(armature->njoints))
				rooti = update(action, rooti, core::maths::Matrix4x4f::identity(), framei);

			for (int i = 0; i < static_cast<int>(armature->njoints); i++)
				matrix_pallet[i] = matrices[i] * armature->joints[i].inv_matrix;
		}
		int update(const Action & action, const int mei, const core::maths::Matrix4x4f & parent_matrix, const int framei)
		{
			const auto pos = action.frames[framei].channels[mei].translation;
			const auto rot = action.frames[framei].channels[mei].rotation;
			const auto scale = action.frames[framei].channels[mei].scale;

			const auto pose =
				make_translation_matrix(pos) *
				make_matrix(rot) *
				make_scale_matrix(scale);

			matrices[mei] = parent_matrix * pose;

			int childi = mei + 1;
			for (int i = 0; i < static_cast<int>(armature->joints[mei].nchildren); i++)
				childi = update(action, childi, matrices[mei], framei);
			return childi;
		}
	};

	core::container::UnorderedCollection
	<
		engine::extras::Asset,
		101,
		std::array<Armature, 50>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	resources;

	core::container::Collection
	<
		engine::Entity,
		201,
		std::array<Character, 100>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	components;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::model::armature>,
	                                 20> queue_add_armature;
	core::container::CircleQueueSRMW<engine::Entity,
	                                 20> queue_remove;
	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::model::action>,
	                                 50> queue_update_action;
}

namespace engine
{
	namespace model
	{
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
					read_armature(file, arm);
					resources.add(armasset, std::move(arm));
				}
				components.add(message_add_armature.first, Character::SetArmature{resources.get<Armature>(armasset)});
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
				components.update(message_update_action.first, Character::SetAction{message_update_action.second.name});
			}

			// update stuff
			for (auto && character : components.get<Character>())
			{
				character.update();
			}

			// send messages
			for (auto && character : components.get<Character>())
			{
				engine::graphics::renderer::update(components.get_key(character),
				                                   engine::graphics::renderer::CharacterSkinning{character.matrix_pallet});
			}
		}

		void add(engine::Entity entity, const armature & data)
		{
			const auto res = queue_add_armature.try_push(std::make_pair(entity, data));
			debug_assert(res);
		}
		void remove(engine::Entity entity)
		{
			const auto res = queue_remove.try_push(entity);
			debug_assert(res);
		}
		void update(engine::Entity entity, const action & data)
		{
			const auto res = queue_update_action.try_push(std::make_pair(entity, data));
			debug_assert(res);
		}
	}
}
