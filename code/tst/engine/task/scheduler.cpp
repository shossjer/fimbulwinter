#include "core/debug.hpp"
#include "core/sync/Event.hpp"

#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"

#include <catch2/catch.hpp>

#include <atomic>

namespace
{
	constexpr int max_threads = 10;
	constexpr int many_tasks = 100;
	constexpr int timeout = 1000;
}

TEST_CASE("task scheduler can be restarted", "[engine][task]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::task::scheduler taskscheduler(max_threads);
	}
}

TEST_CASE("task scheduler", "[engine][task]")
{
	engine::task::scheduler taskscheduler(max_threads);

	struct Data
	{
		engine::task::scheduler & scheduler;
		engine::Hash strand;

		std::atomic_int count{};
		core::sync::Event<true> event;

		explicit Data(engine::task::scheduler & scheduler, engine::Hash strand)
			: scheduler(scheduler)
			, strand(strand)
		{}
	};

	SECTION("can call works without a strand")
	{
		constexpr engine::Hash anystrand = engine::Hash{};

		Data data(taskscheduler, anystrand);

		for (int i = 0; i < many_tasks; i++)
		{
			engine::task::post_work(
				taskscheduler,
				anystrand,
				[](engine::task::scheduler & scheduler, engine::Hash strand, utility::any && data)
				{
					if (!debug_assert(data.type_id() == utility::type_id<Data *>()))
						return;

					Data * const d = utility::any_cast<Data *>(data);

					if (d->scheduler == scheduler &&
					    d->strand == strand)
					{
						d->count++;
						if (d->count == many_tasks)
						{
							d->event.set();
						}
					}
				},
				&data);
		}

		REQUIRE(data.event.wait(timeout));
	}

	SECTION("only calls work one at a time within a strand")
	{
		constexpr engine::Hash mystrand = engine::Hash("mystrand");

		Data data(taskscheduler, mystrand);

		for (int i = 0; i < many_tasks; i++)
		{
			engine::task::post_work(
				taskscheduler,
				mystrand,
				[](engine::task::scheduler & scheduler, engine::Hash strand, utility::any && data)
				{
					if (!debug_assert(data.type_id() == utility::type_id<Data *>()))
						return;

					Data * const d = utility::any_cast<Data *>(data);

					if (d->scheduler == scheduler &&
					    d->strand == strand &&
					    d->count % 2 == 0)
					{
						d->count++;
					}
				},
				&data);

			engine::task::post_work(
				taskscheduler,
				mystrand,
				[](engine::task::scheduler & scheduler, engine::Hash strand, utility::any && data)
				{
					if (!debug_assert(data.type_id() == utility::type_id<Data *>()))
						return;

					Data * const d = utility::any_cast<Data *>(data);

					if (d->scheduler == scheduler &&
					    d->strand == strand &&
					    d->count % 2 == 1)
					{
						d->count++;
						if (d->count == 2 * many_tasks)
						{
							d->event.set();
						}
					}
				},
				&data);
		}

		REQUIRE(data.event.wait(timeout));
	}
}
