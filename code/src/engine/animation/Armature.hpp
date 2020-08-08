#pragma once

#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>
#include "core/serialization.hpp"

#include <cstdint>
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

				core::maths::Matrix4x4f inv_matrix;

				uint16_t parenti;
				uint16_t nchildren;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("name"), &Joint::name),
						std::make_pair(utility::string_units_utf8("inv_matrix"), &Joint::inv_matrix),
						std::make_pair(utility::string_units_utf8("parent"), &Joint::parenti),
						std::make_pair(utility::string_units_utf8("children"), &Joint::nchildren)
						);
				}
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

						static constexpr auto serialization()
						{
							return utility::make_lookup_table(
								std::make_pair(utility::string_units_utf8("translation"), &Channel::translation),
								std::make_pair(utility::string_units_utf8("rotation"), &Channel::rotation),
								std::make_pair(utility::string_units_utf8("scale"), &Channel::scale)
								);
						}
					};

					std::vector<Channel> channels;

					Frame(std::size_t nchannels) :
						channels(nchannels)
					{}

					static constexpr auto serialization()
					{
						return utility::make_lookup_table(
							std::make_pair(utility::string_units_utf8("channels"), &Frame::channels)
							);
					}
				};

				char name[24]; // arbitrary

				int32_t length;

				std::vector<Frame> frames;
				std::vector<core::maths::Vector3f> positions;
				std::vector<core::maths::Quaternionf> orientations;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("name"), &Action::name),
						std::make_pair(utility::string_units_utf8("length"), &Action::length),
						std::make_pair(utility::string_units_utf8("frames"), &Action::frames),
						std::make_pair(utility::string_units_utf8("positions"), &Action::positions),
						std::make_pair(utility::string_units_utf8("orientations"), &Action::orientations)
						);
				}
			};

			char name[64]; // arbitrary

			std::vector<Joint> joints;
			std::vector<Action> actions;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("name"), &Armature::name),
					std::make_pair(utility::string_units_utf8("joints"), &Armature::joints),
					std::make_pair(utility::string_units_utf8("actions"), &Armature::actions)
					);
			}
		};
	}
}
