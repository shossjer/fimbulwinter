
#ifndef ENGINE_GRAPHICS_EVENTS_HPP
#define ENGINE_GRAPHICS_EVENTS_HPP

#include "renderer.hpp"

#include <core/maths/Matrix.hpp>

namespace engine
{
	namespace graphics
	{
		struct ModelMatrixMessage
		{
			std::size_t object;
			core::maths::Matrixf model_matrix;
		};
		struct RectangleMessage
		{
			std::size_t object;
			Rectangle rectangle;
		};
		struct RemoveMessage
		{
			std::size_t object;
		};

		void post_message(const ModelMatrixMessage & message);
		void post_message(const RectangleMessage & message);
		void post_message(const RemoveMessage & message);
	}
}

#endif /* ENGINE_GRAPHICS_EVENTS_HPP */
