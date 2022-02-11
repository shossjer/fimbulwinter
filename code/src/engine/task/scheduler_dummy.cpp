#include "config.h"

#if TASK_USE_DUMMY

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"
#include "utility/optional.hpp"
#include "utility/spinlock.hpp"
#include "utility/variant.hpp"

#include <mutex>

namespace
{
	struct Work
	{
		engine::Hash strand;
		engine::task::work_callback * workcall;
		utility::any data;
	};

	struct Terminate
	{
	};

	using Message = utility::variant
	<
		Work,
		Terminate
	>;
}

namespace engine
{
	namespace task
	{
		struct scheduler_impl
		{
			core::container::PageQueue<utility::heap_storage<Message>> queue;

			core::async::Thread thread;
			core::sync::Event<true> event;
		};
	}
}

namespace
{
	utility::spinlock singelton_lock;
	utility::optional<engine::task::scheduler_impl> singelton;

	engine::task::scheduler_impl * create_impl()
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		if (singelton)
			return nullptr;

		singelton.emplace();

		return &singelton.value();
	}

	void destroy_impl(engine::task::scheduler_impl & /*impl*/)
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		singelton.reset();
	}
}

namespace
{
	bool parse_message(engine::task::scheduler_impl & impl)
	{
		Message message;
		while (impl.queue.try_pop(message))
		{
			struct
			{
				engine::task::scheduler_impl & impl;

				bool operator () (Work && x)
				{
					engine::task::scheduler scheduler(impl);
					x.workcall(scheduler, x.strand, std::move(x.data));
					scheduler.detach();

					return true;
				}

				bool operator () (Terminate &&) { return false; }

			} visitor{impl};

			if (!visit(visitor, std::move(message)))
				return false;
		}
		return true;
	}

	core::async::thread_return thread_decl scheduler_thread(core::async::thread_param arg)
	{
		engine::task::scheduler_impl & impl = *static_cast<engine::task::scheduler_impl *>(arg);

		do
		{
			impl.event.wait();
			impl.event.reset();
		}
		while (parse_message(impl));

		return core::async::thread_return{};
	}
}

namespace engine
{
	namespace task
	{
		scheduler_impl * scheduler::construct(ext::ssize thread_count)
		{
			if (!debug_verify(0 < thread_count))
				return nullptr;

			scheduler_impl * const impl = create_impl();
			if (debug_verify(impl))
			{
				impl->thread = core::async::Thread(scheduler_thread, impl);
			}
			return impl;
		}

		void scheduler::destruct(scheduler_impl & impl)
		{
			if (debug_verify(impl.queue.try_emplace(utility::in_place_type<Terminate>)))
			{
				impl.event.set();
				impl.thread.join();
			}
			impl.event.reset();

			destroy_impl(impl);
		}

		void post_work(
			scheduler & scheduler,
			engine::Hash strand,
			work_callback * workcall,
			utility::any && data)
		{
			if (debug_verify(scheduler->queue.try_emplace(utility::in_place_type<Work>, strand, workcall, std::move(data))))
			{
				scheduler->event.set();
			}
		}
	}
}

#endif
