
#ifndef GAMEPLAY_LEVEL_PLACEHOLDER_HPP
#define GAMEPLAY_LEVEL_PLACEHOLDER_HPP

#include <engine/common.hpp>

#include <string>

namespace gameplay
{
namespace level
{
	struct placeholder_t
	{
		std::string name;
		engine::transform_t transform;
		core::maths::Vector3f scale;
	};

	engine::Entity load(const placeholder_t & placeholder);
}
}

#endif // GAMEPLAY_LEVEL_PLACEHOLDERS_HPP
