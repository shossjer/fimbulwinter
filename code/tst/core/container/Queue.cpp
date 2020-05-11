#include "core/container/Queue.hpp"

#include <catch/catch.hpp>

static_assert(!std::is_trivially_destructible<core::container::SimpleQueue<utility::heap_storage<int>>>::value, "");
static_assert(!std::is_trivially_destructible<core::container::SimpleQueue<utility::heap_storage<int, float, long double>>>::value, "");
static_assert(!std::is_trivially_destructible<core::container::SimpleQueue<utility::heap_storage<int, std::string>>>::value, "");

static_assert(std::is_trivially_destructible<core::container::SimpleQueue<utility::static_storage<10, int>>>::value, "");
static_assert(std::is_trivially_destructible<core::container::SimpleQueue<utility::static_storage<10, int, float, long double>>>::value, "");
static_assert(!std::is_trivially_destructible<core::container::SimpleQueue<utility::static_storage<10, int, std::string>>>::value, "");

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::heap_storage<int>> q(4);
	CHECK(q.capacity() == 3);

	SECTION("")
	{
		int v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7)));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(v));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(std::move(v)));
		}

		int v = 0;
		REQUIRE(q.try_pop(v));
		CHECK(v == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(1));
		REQUIRE(q.try_emplace(2));
		REQUIRE(q.try_emplace(3));

		SECTION("")
		{
			CHECK_FALSE(q.try_emplace(7));
		}

		SECTION("")
		{
			int v = 0;
			REQUIRE(q.try_pop(v));
			CHECK(v == 1);

			SECTION("")
			{
				REQUIRE(q.try_emplace(4));

				CHECK_FALSE(q.try_emplace(7));
			}

			SECTION("")
			{
				REQUIRE(q.try_pop(v));
				CHECK(v == 2);

				REQUIRE(q.try_pop(v));
				CHECK(v == 3);

				CHECK_FALSE(q.try_pop(v));
			}
		}
	}
}

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::static_storage<4, int>> q;
	CHECK(q.capacity() == 3);

	SECTION("")
	{
		int v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7)));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(v));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(std::move(v)));
		}

		int v = 0;
		REQUIRE(q.try_pop(v));
		CHECK(v == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(1));
		REQUIRE(q.try_emplace(2));
		REQUIRE(q.try_emplace(3));

		SECTION("")
		{
			CHECK_FALSE(q.try_emplace(7));
		}

		SECTION("")
		{
			int v = 0;
			REQUIRE(q.try_pop(v));
			CHECK(v == 1);

			SECTION("")
			{
				REQUIRE(q.try_emplace(4));

				CHECK_FALSE(q.try_emplace(7));
			}

			SECTION("")
			{
				REQUIRE(q.try_pop(v));
				CHECK(v == 2);

				REQUIRE(q.try_pop(v));
				CHECK(v == 3);

				CHECK_FALSE(q.try_pop(v));
			}
		}
	}
}

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::static_storage<4, int>> q(4);
	CHECK(q.capacity() == 3);
}

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::heap_storage<int, std::string, std::unique_ptr<int>>> q(4);
	CHECK(q.capacity() == 3);

	SECTION("")
	{
		std::tuple<int, std::string, std::unique_ptr<int>> v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7), std::forward_as_tuple("77", 1), std::forward_as_tuple(std::make_unique<int>(7))));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(7, "7", std::make_unique<int>(7));
			REQUIRE(q.try_push(std::move(v)));
		}

		std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
		REQUIRE(q.try_pop(v));
		CHECK(std::get<0>(v) == 7);
		CHECK(std::get<1>(v) == "7");
		REQUIRE(std::get<2>(v));
		CHECK(*std::get<2>(v) == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(1, "1", std::make_unique<int>(1)));
		REQUIRE(q.try_emplace(2, "2", std::make_unique<int>(2)));
		REQUIRE(q.try_emplace(3, "3", std::make_unique<int>(3)));

		SECTION("")
		{
			CHECK_FALSE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
			REQUIRE(q.try_pop(v));
			CHECK(std::get<0>(v) == 1);
			CHECK(std::get<1>(v) == "1");
			REQUIRE(std::get<2>(v));
			CHECK(*std::get<2>(v) == 1);

			SECTION("")
			{
				REQUIRE(q.try_emplace(4, "4", std::make_unique<int>(4)));

				CHECK_FALSE(q.try_emplace(7, "7", std::make_unique<int>(7)));
			}

			SECTION("")
			{
				REQUIRE(q.try_pop(v));
				CHECK(std::get<0>(v) == 2);
				CHECK(std::get<1>(v) == "2");
				REQUIRE(std::get<2>(v));
				CHECK(*std::get<2>(v) == 2);

				REQUIRE(q.try_pop(v));
				CHECK(std::get<0>(v) == 3);
				CHECK(std::get<1>(v) == "3");
				REQUIRE(std::get<2>(v));
				CHECK(*std::get<2>(v) == 3);

				CHECK_FALSE(q.try_pop(v));
			}
		}
	}
}

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::static_storage<4, int, std::string, std::unique_ptr<int>>> q;
	CHECK(q.capacity() == 3);

	SECTION("")
	{
		std::tuple<int, std::string, std::unique_ptr<int>> v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7), std::forward_as_tuple("77", 1), std::forward_as_tuple(std::make_unique<int>(7))));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(7, "7", std::make_unique<int>(7));
			REQUIRE(q.try_push(std::move(v)));
		}

		std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
		REQUIRE(q.try_pop(v));
		CHECK(std::get<0>(v) == 7);
		CHECK(std::get<1>(v) == "7");
		REQUIRE(std::get<2>(v));
		CHECK(*std::get<2>(v) == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(1, "1", std::make_unique<int>(1)));
		REQUIRE(q.try_emplace(2, "2", std::make_unique<int>(2)));
		REQUIRE(q.try_emplace(3, "3", std::make_unique<int>(3)));

		SECTION("")
		{
			CHECK_FALSE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
			REQUIRE(q.try_pop(v));
			CHECK(std::get<0>(v) == 1);
			CHECK(std::get<1>(v) == "1");
			REQUIRE(std::get<2>(v));
			CHECK(*std::get<2>(v) == 1);

			SECTION("")
			{
				REQUIRE(q.try_emplace(4, "4", std::make_unique<int>(4)));

				CHECK_FALSE(q.try_emplace(7, "7", std::make_unique<int>(7)));
			}

			SECTION("")
			{
				REQUIRE(q.try_pop(v));
				CHECK(std::get<0>(v) == 2);
				CHECK(std::get<1>(v) == "2");
				REQUIRE(std::get<2>(v));
				CHECK(*std::get<2>(v) == 2);

				REQUIRE(q.try_pop(v));
				CHECK(std::get<0>(v) == 3);
				CHECK(std::get<1>(v) == "3");
				REQUIRE(std::get<2>(v));
				CHECK(*std::get<2>(v) == 3);

				CHECK_FALSE(q.try_pop(v));
			}
		}
	}
}

TEST_CASE("", "")
{
	core::container::SimpleQueue<utility::static_storage<4, int, std::string, std::unique_ptr<int>>> q(4);
	CHECK(q.capacity() == 3);
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::heap_storage<int>> q;

	SECTION("")
	{
		int v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7)));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(v));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(std::move(v)));
		}

		int v = 0;
		REQUIRE(q.try_pop(v));
		CHECK(v == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7));
	}
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::heap_storage<int>> q(9);

	SECTION("")
	{
		int v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7)));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(v));
		}

		SECTION("")
		{
			int v = 7;
			REQUIRE(q.try_push(std::move(v)));
		}

		int v = 0;
		REQUIRE(q.try_pop(v));
		CHECK(v == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7));
	}
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::static_storage<4, int>> q;

	SECTION("")
	{
		int v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		CHECK_FALSE(q.try_emplace(7));
	}
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::heap_storage<int, std::string, std::unique_ptr<int>>> q;

	SECTION("")
	{
		std::tuple<int, std::string, std::unique_ptr<int>> v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7), std::forward_as_tuple("77", 1), std::forward_as_tuple(std::make_unique<int>(7))));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(7, "7", std::make_unique<int>(7));
			REQUIRE(q.try_push(std::move(v)));
		}

		std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
		REQUIRE(q.try_pop(v));
		CHECK(std::get<0>(v) == 7);
		CHECK(std::get<1>(v) == "7");
		REQUIRE(std::get<2>(v));
		CHECK(*std::get<2>(v) == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
	}
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::heap_storage<int, std::string, std::unique_ptr<int>>> q(9);

	SECTION("")
	{
		std::tuple<int, std::string, std::unique_ptr<int>> v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		SECTION("")
		{
			REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
		}

		SECTION("")
		{
			REQUIRE(q.try_emplace(std::piecewise_construct, std::forward_as_tuple(7), std::forward_as_tuple("77", 1), std::forward_as_tuple(std::make_unique<int>(7))));
		}

		SECTION("")
		{
			std::tuple<int, std::string, std::unique_ptr<int>> v(7, "7", std::make_unique<int>(7));
			REQUIRE(q.try_push(std::move(v)));
		}

		std::tuple<int, std::string, std::unique_ptr<int>> v(0, "", nullptr);
		REQUIRE(q.try_pop(v));
		CHECK(std::get<0>(v) == 7);
		CHECK(std::get<1>(v) == "7");
		REQUIRE(std::get<2>(v));
		CHECK(*std::get<2>(v) == 7);

		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		REQUIRE(q.try_emplace(7, "7", std::make_unique<int>(7)));
	}
}

TEST_CASE("", "")
{
	core::container::PageQueue<utility::static_storage<4, int, std::string, std::unique_ptr<int>>> q;

	SECTION("")
	{
		std::tuple<int, std::string, std::unique_ptr<int>> v;
		CHECK_FALSE(q.try_pop(v));
	}

	SECTION("")
	{
		CHECK_FALSE(q.try_emplace(7, "7", std::make_unique<int>(7)));
	}
}
