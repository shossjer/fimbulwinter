
#ifndef ENGINE_PHYSICS_HELPER_PHYSX_HPP
#define ENGINE_PHYSICS_HELPER_PHYSX_HPP

#include <PxPhysicsAPI.h>

#include <core/maths/Vector.hpp>

/**
 *	\note Should be used by physics implementation only.
 */
namespace engine
{
namespace physics
{
	template<class T>
	inline T convert(const core::maths::Vector3f val)
	{
		core::maths::Vector3f::array_type buffer;
		val.get_aligned(buffer);
		return T {buffer[0], buffer[1], buffer[2]};
	}

	inline physx::PxQuat convert(const core::maths::Quaternionf val)
	{
		core::maths::Quaternionf::array_type buffer;
		val.get_aligned(buffer);
		return physx::PxQuat { buffer[1], buffer[2], buffer[3], buffer[0] };
	}

	inline core::maths::Quaternionf convert(const physx::PxQuat val)
	{
		return core::maths::Quaternionf {val.w, val.x, val.y, val.z};
	}

	inline core::maths::Vector3f convert(const physx::PxVec3 val)
	{
		return core::maths::Vector3f {val.x, val.y, val.z};
	}

	inline core::maths::Vector3f convert(const physx::PxTransform val)
	{
		return core::maths::Vector3f {val.p.x, val.p.y, val.p.z};
	}

	inline core::maths::Vector3f convert(const physx::PxExtendedVec3 val)
	{
		return core::maths::Vector3f {(float) val.x, (float) val.y, (float) val.z};
	}
}
}

#endif /* ENGINE_PHYSICS_HELPER_PHYSX_HPP */
