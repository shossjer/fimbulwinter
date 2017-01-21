
#ifndef GAMEPLAY_LEVEL_PLACEHOLDER_HPP
#define GAMEPLAY_LEVEL_PLACEHOLDER_HPP

#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <string>

namespace gameplay
{
namespace level
{
	struct placeholder_t
	{
		std::string name;
		core::maths::Vector3f pos;
		core::maths::Quaternionf quat;
	};

	void load(const placeholder_t & placeholder);
}
}

#endif // GAMEPLAY_LEVEL_PLACEHOLDERS_HPP
