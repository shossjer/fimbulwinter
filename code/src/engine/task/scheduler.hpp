#pragma once

#include "engine/Hash.hpp"

namespace utility
{
	class any;
}

namespace engine
{
	namespace task
	{
		struct scheduler_impl;

		struct scheduler
		{
			scheduler_impl * ptr;

			~scheduler();
			explicit scheduler(scheduler_impl & impl);
			explicit scheduler(ext::ssize thread_count);
		};

		using work_callback = void(
			scheduler & scheduler,
			engine::Hash strand,
			utility::any && data);

		void post_work(
			scheduler & scheduler,
			engine::Hash strand,
			work_callback * workcall,
			utility::any && data);
	}
}
