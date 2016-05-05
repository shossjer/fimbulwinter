
#include "events.hpp"

#include "renderer.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/debug.hpp>

namespace
{
	core::container::CircleQueue<engine::graphics::ModelMatrixMessage, 100> model_matrix_message_queue;
	core::container::CircleQueue<engine::graphics::RectangleMessage, 100> rectangle_message_queue;
	core::container::CircleQueue<engine::graphics::RemoveMessage, 100> remove_message_queue;
}

namespace engine
{
	namespace graphics
	{
		void post_message(const ModelMatrixMessage & message)
		{
			model_matrix_message_queue.try_push(message);
		}
		void post_message(const RectangleMessage & message)
		{
			rectangle_message_queue.try_push(message);
		}
		void post_message(const RemoveMessage & message)
		{
			remove_message_queue.try_push(message);
		}

		void poll_messages()
		{
			{
				ModelMatrixMessage message;
				while (model_matrix_message_queue.try_pop(message))
					renderer::set(message);
			}
			{
				RectangleMessage message;
				while (rectangle_message_queue.try_pop(message))
					renderer::add(message.object, message.rectangle);
			}
			{
				RemoveMessage message;
				while (remove_message_queue.try_pop(message))
					renderer::remove(message.object);
			}
		}
	}
}
