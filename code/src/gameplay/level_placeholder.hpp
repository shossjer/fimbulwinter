
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

	void load(const engine::Entity id, const std::string & name, const Matrix4x4f & matrix);

	engine::Entity load(const placeholder_t & placeholder);
}
}

#endif // GAMEPLAY_LEVEL_PLACEHOLDERS_HPP
