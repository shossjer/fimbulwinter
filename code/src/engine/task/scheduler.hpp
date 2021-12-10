#pragma once

#include "engine/Hash.hpp"
#include "engine/module.hpp"

#include "utility/ext/stddef.hpp"

namespace utility
{
	class any;
}

namespace engine
{
	namespace task
	{
		struct scheduler_impl;

		struct scheduler : module<scheduler, scheduler_impl>
		{
			using module<scheduler, scheduler_impl>::module;

			static scheduler_impl * construct(ext::ssize thread_count);
			static void destruct(scheduler_impl & impl);
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
