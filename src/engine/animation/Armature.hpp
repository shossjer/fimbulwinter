
#ifndef ENGINE_ANIMATION_ARMATURE_HPP
#define ENGINE_ANIMATION_ARMATURE_HPP

#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace engine
{
	namespace animation
	{
		struct Armature
		{
			struct Joint
			{
				char name[16]; // arbitrary

				// core::maths::Matrix4x4f matrix; // unused
				core::maths::Matrix4x4f inv_matrix;

				uint16_t parenti;
				uint16_t nchildren;
			};

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

			char name[64]; // arbitrary

			uint16_t njoints;
			uint16_t nactions;

			std::vector<Joint> joints;
			std::vector<Action> actions;

			/**
			 */
			void read(std::ifstream & stream);
		};
	}
}

#endif /* ENGINE_ANIMATION_ARMATURE_HPP */
