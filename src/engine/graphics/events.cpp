
#include "events.hpp"

#include <core/debug.hpp>

#include <atomic>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			extern void set(const ModelViewMessage & message);
		}
	}
}

namespace
{
	template <typename T, std::size_t N>
	class CircleQueue
	{
	public:
		static constexpr std::size_t capacity = N;

	public:
		std::atomic_int begini;
		std::atomic_int endi;
		T buffer[N];

	public:
		CircleQueue() :
			begini(0),
			endi(0)
		{
		}

	public:
		// std::size_t size() const
		// {
		// 	if (begini < endi)
		// 		return endi - begini;
		// 	return endi - begini + N;
		// }

		void push_back(const T & item)
		{
			const int next_endi = this->endi == this->capacity - 1 ? 0 : this->endi + 1;
			debug_assert(next_endi != this->begini); // we cannot wrap around
			this->buffer[this->endi] = item;
			this->endi = next_endi;
		}
	};

	CircleQueue<engine::graphics::ModelViewMessage, 100> model_view_message_queue;
}

namespace engine
{
	namespace graphics
	{
		void post_message(const ModelViewMessage & message)
		{
			model_view_message_queue.push_back(message);
		}

		void poll_messages()
		{
			int begini = model_view_message_queue.begini;
			const int endi = model_view_message_queue.endi;

			while (begini != endi)
			{
				renderer::set(model_view_message_queue.buffer[begini]);

				begini++;
				if (begini == model_view_message_queue.capacity)
					begini = 0;
			}
			model_view_message_queue.begini = begini;
		}
	}
}
