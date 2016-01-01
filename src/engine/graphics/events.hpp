
#ifndef ENGINE_GRAPHICS_EVENTS_HPP
#define ENGINE_GRAPHICS_EVENTS_HPP

#include <core/maths/Matrix.hpp>

namespace engine
{
	namespace graphics
	{
		struct ModelViewMessage
		{
			std::size_t object;
			core::maths::Matrixf model_view;
		};

		void post_message(const ModelViewMessage & message);
	}
}

#endif /* ENGINE_GRAPHICS_EVENTS_HPP */
