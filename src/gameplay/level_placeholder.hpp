
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
		core::maths::Vector3f scale;
	};

	void load(const placeholder_t & placeholder, const std::string & type);
}
}

#endif // GAMEPLAY_LEVEL_PLACEHOLDERS_HPP
