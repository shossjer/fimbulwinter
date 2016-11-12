
#ifndef ENGINE_PHYSICS_MATERIAL_DATA_HPP
#define ENGINE_PHYSICS_MATERIAL_DATA_HPP

namespace engine
{
namespace physics
{
	// Material properties, used when creating actors
	struct MaterialData
	{
		const float density;
		const float friction;
		const float restitution;

		MaterialData(const float density, const float friction, const float restitution)
			:
			density(density),
			friction(friction),
			restitution(restitution)
		{
		}
	};
}
}

#endif // ENGINE_PHYSICS_MATERIAL_DATA_HPP