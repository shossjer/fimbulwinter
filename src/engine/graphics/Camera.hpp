
#ifndef ENGINE_GRAPHICS_CAMERA_HPP
#define ENGINE_GRAPHICS_CAMERA_HPP

#include <core/maths/Vector.hpp>

#include <array>

namespace engine
{
namespace graphics
{
	class Camera
	{
	//	using Vec = ::core::maths::Vector3f;
		
		using Vec = std::array<float, 3>;
		/**
		 */
		Vec pos;
		/**
		 */
		// Vec dir; // not used

	public:

		Camera() : pos{{0.f, 0.f, 0.f}}
		{

		}

	public:

		float getX() const { return pos[0]; }
		float getY() const { return pos[1]; }
		float getZ() const { return pos[2]; }

	//	const Vec & position() const { return this->pos; }
	//	const Vec & direction() const { return this->dir; }
		/**
		 */
		void position(const float x, const float y, const float z)
		{
			this->pos[0] = x;
			this->pos[1] = y;
			this->pos[2] = z;
		}
		///**
		// */
		//void direction(Vec & dir)
		//{
		//	this->dir = dir;
		//}
	};
}
}

#endif /* ENGINE_GRAPHICS_CAMERA_HPP */
