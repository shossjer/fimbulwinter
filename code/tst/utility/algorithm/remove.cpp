#include "utility/algorithm/remove.hpp"

#include <catch2/catch.hpp>

TEST_CASE("remove_if", "[utility][algorithm]")
{
	SECTION("with even size input")
	{
		std::vector<int> numbers = {1, 2, 3, 4, 6, 7, 8, 9};

		numbers.erase(
			ext::remove_if(numbers, [](int n) { return (n & 1) == 1; }),
			numbers.end());

		REQUIRE(numbers.size() == 4);
		// todo order independent
		CHECK(numbers[0] == 8);
		CHECK(numbers[1] == 2);
		CHECK(numbers[2] == 6);
		CHECK(numbers[3] == 4);
	}

	SECTION("with odd size input")
	{
		std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

		numbers.erase(
			ext::remove_if(numbers, [](int n) { return (n & 1) == 1; }),
			numbers.end());

		REQUIRE(numbers.size() == 4);
		// todo order independent
		CHECK(numbers[0] == 8);
		CHECK(numbers[1] == 2);
		CHECK(numbers[2] == 6);
		CHECK(numbers[3] == 4);
	}
}

TEST_CASE("remove_copy_if", "[utility][algorithm]")
{
	SECTION("with even size input")
	{
		std::vector<int> numbers = {1, 2, 3, 4, 6, 7, 8, 9};
		int copy[9] = {};

		numbers.erase(
			ext::remove_copy_if(numbers, copy, [](int n) { return (n & 1) == 1; }),
			numbers.end());

		REQUIRE(numbers.size() == 4);
		// todo order independent
		CHECK(numbers[0] == 8);
		CHECK(numbers[1] == 2);
		CHECK(numbers[2] == 6);
		CHECK(numbers[3] == 4);

		CHECK(copy[0] == 1);
		CHECK(copy[1] == 9);
		CHECK(copy[2] == 3);
		CHECK(copy[3] == 7);
		CHECK(copy[4] == 0);
		CHECK(copy[5] == 0);
		CHECK(copy[6] == 0);
		CHECK(copy[7] == 0);
		CHECK(copy[8] == 0);
	}

	SECTION("with odd size input")
	{
		std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};
		int copy[10] = {};

		numbers.erase(
			ext::remove_copy_if(numbers, copy, [](int n) { return (n & 1) == 1; }),
			numbers.end());

		REQUIRE(numbers.size() == 4);
		// todo order independent
		CHECK(numbers[0] == 8);
		CHECK(numbers[1] == 2);
		CHECK(numbers[2] == 6);
		CHECK(numbers[3] == 4);

		CHECK(copy[0] == 1);
		CHECK(copy[1] == 9);
		CHECK(copy[2] == 3);
		CHECK(copy[3] == 7);
		CHECK(copy[4] == 5);
		CHECK(copy[5] == 0);
		CHECK(copy[6] == 0);
		CHECK(copy[7] == 0);
		CHECK(copy[8] == 0);
		CHECK(copy[9] == 0);
	}
}
