#include "utility/any.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <utility>

TEST_CASE("any empty", "[utility][any]")
{
	utility::any a1;
	CHECK(!a1.has_value());
	CHECK(a1.type_id() == utility::type_id<void>());

	SECTION("can be copy constructed")
	{
		utility::any a2 = a1;
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());

		SECTION("and copy assigned")
		{
			a1 = a2;
			CHECK(!a1.has_value());
			CHECK(a1.type_id() == utility::type_id<void>());
			CHECK(!a2.has_value());
			CHECK(a2.type_id() == utility::type_id<void>());

			SECTION("even to itself")
			{
				a1 = a1;
				CHECK(!a1.has_value());
				CHECK(a1.type_id() == utility::type_id<void>());
			}
		}
	}

	SECTION("can be move constructed")
	{
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());

		SECTION("and move assigned")
		{
			a1 = std::move(a2);
			CHECK(!a1.has_value());
			CHECK(a1.type_id() == utility::type_id<void>());
			CHECK(!a2.has_value());
			CHECK(a2.type_id() == utility::type_id<void>());

			SECTION("even to itself")
			{
				a1 = std::move(a1);
				CHECK(!a1.has_value());
				CHECK(a1.type_id() == utility::type_id<void>());
			}
		}
	}

	SECTION("can be swapped")
	{
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
}

TEST_CASE("any small", "[utility][any]")
{
	static_assert(std::is_same<utility::detail::any_small<int>, utility::detail::any_data::any_type<int>>::value, "expected int to be a small type");

	utility::any a1(utility::in_place_type<int>, 1);
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 1);

	SECTION("can be conversion constructed")
	{
		utility::any a2 = int{1};
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);

		SECTION("and conversion assigned")
		{
			a2 = int{11};
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a2) == 11);
		}
	}

	SECTION("can be copy constructed")
	{
		utility::any a2 = a1;
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);

		SECTION("and copy assigned")
		{
			a1 = a2;
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a1) == 1);
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a2) == 1);

			a1.reset();
			a1 = a2;
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a1) == 1);
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a2) == 1);

			SECTION("even to itself")
			{
				a1 = a1;
				CHECK(a1.has_value());
				REQUIRE(a1.type_id() == utility::type_id<int>());
				CHECK(utility::any_cast<int>(a1) == 1);
			}
		}
	}

	SECTION("can be move constructed")
	{
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);

		SECTION("and move assigned")
		{
			a1 = std::move(a2);
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<int>());
			CHECK(utility::any_cast<int>(a1) == 1);
			CHECK(!a2.has_value());
			CHECK(a2.type_id() == utility::type_id<void>());

			SECTION("even to itself")
			{
				a1 = std::move(a1);
				CHECK(!a1.has_value());
				CHECK(a1.type_id() == utility::type_id<void>());
			}
		}
	}

	SECTION("can be swapped")
	{
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);

		a1 = 11;
		swap(a1, a2);
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 11);
	}
}

TEST_CASE("any big", "[utility][any]")
{
	static_assert(std::is_same<utility::detail::any_big<std::string>, utility::detail::any_data::any_type<std::string>>::value, "expected std::string to be a big type");

	utility::any a1(utility::in_place_type<std::string>, "111");
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<std::string>());
	CHECK(utility::any_cast<std::string>(a1) == "111");

	SECTION("can be conversion constructed")
	{
		utility::any a2 = std::string("111");
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a2) == "111");

		SECTION("and conversion assigned")
		{
			a2 = std::string("111111");
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a2) == "111111");
		}
	}

	SECTION("can be copy constructed")
	{
		utility::any a2 = a1;
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a1) == "111");
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a2) == "111");

		SECTION("and copy assigned")
		{
			a1 = a2;
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a1) == "111");
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a2) == "111");

			a1.reset();
			a1 = a2;
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a1) == "111");
			CHECK(a2.has_value());
			REQUIRE(a2.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a2) == "111");

			SECTION("even to itself")
			{
				a1 = a1;
				CHECK(a1.has_value());
				REQUIRE(a1.type_id() == utility::type_id<std::string>());
				CHECK(utility::any_cast<std::string>(a1) == "111");
			}
		}
	}

	SECTION("can be move constructed")
	{
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a2) == "111");

		SECTION("and move assigned")
		{
			a1 = std::move(a2);
			CHECK(a1.has_value());
			REQUIRE(a1.type_id() == utility::type_id<std::string>());
			CHECK(utility::any_cast<std::string>(a1) == "111");
			CHECK(!a2.has_value());
			CHECK(a2.type_id() == utility::type_id<void>());

			SECTION("even to itself")
			{
				a1 = std::move(a1);
				CHECK(!a1.has_value());
				CHECK(a1.type_id() == utility::type_id<void>());
			}
		}
	}

	SECTION("can be swapped")
	{
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a2) == "111");

		a1 = std::string("111111");
		swap(a1, a2);
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a1) == "111");
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<std::string>());
		CHECK(utility::any_cast<std::string>(a2) == "111111");
	}
}

TEST_CASE( "any emplace", "[utility][any]" )
{
	utility::any a1;
	int & i1 = a1.emplace<int>(1);
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 1);
	i1 = 2;
	CHECK(utility::any_cast<int>(a1) == 2);
	a1.emplace<int>(3);
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 3);
}

TEST_CASE( "any reset", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
	}
}
