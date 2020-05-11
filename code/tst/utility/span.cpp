#include "utility/span.hpp"

#include <catch/catch.hpp>

#define CHECK_SPAN_0(var, arr) \
	CHECK(var.size() == 0); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr); \
	CHECK(utility::to_address(var.rbegin()) + 1 == arr); \
	CHECK(utility::to_address(var.rend()) + 1 == arr)

#define CHECK_SPAN_1(var, arr) \
	CHECK(var.size() == 1); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr + 1); \
	CHECK(utility::to_address(var.rbegin()) == arr); \
	CHECK(utility::to_address(var.rend()) + 1 == arr); \
	CHECK(var[0] == 1)

#define CHECK_SPAN_2(var, arr) \
	CHECK(var.size() == 2); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr + 2); \
	CHECK(utility::to_address(var.rbegin()) == arr + 1); \
	CHECK(utility::to_address(var.rend()) + 1 == arr); \
	CHECK(var[0] == 1); \
	CHECK(var[1] == 2)

#define CHECK_SPAN_3(var, arr) \
	CHECK(var.size() == 3); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr + 3); \
	CHECK(utility::to_address(var.rbegin()) == arr + 2); \
	CHECK(utility::to_address(var.rend()) + 1 == arr); \
	CHECK(var[0] == 1); \
	CHECK(var[2] == 3)

#define CHECK_SPAN_4(var, arr) \
	CHECK(var.size() == 4); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr + 4); \
	CHECK(utility::to_address(var.rbegin()) == arr + 3); \
	CHECK(utility::to_address(var.rend()) + 1 == arr); \
	CHECK(var[0] == 1); \
	CHECK(var[3] == 4)

#define CHECK_SPAN_5(var, arr) \
	CHECK(var.size() == 5); \
	CHECK(var.data() == arr); \
	CHECK(utility::to_address(var.begin()) == arr); \
	CHECK(utility::to_address(var.end()) == arr + 5); \
	CHECK(utility::to_address(var.rbegin()) == arr + 4); \
	CHECK(utility::to_address(var.rend()) + 1 == arr); \
	CHECK(var[0] == 1); \
	CHECK(var[4] == 5)

TEST_CASE("", "[utility][span]")
{
	utility::span<int> s;
	CHECK(s.size() == 0);
	CHECK(s.begin() == s.end());
	CHECK(s.rbegin() == s.rend());
}

TEST_CASE("", "[utility][span]")
{
	utility::span<int, 0> s;
	CHECK(s.size() == 0);
	CHECK(s.begin() == s.end());
	CHECK(s.rbegin() == s.rend());
}

TEST_CASE("", "[utility][span]")
{
	int a[] = {1, 2, 3, 4, 5};

	SECTION("")
	{
		utility::span<int> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<int, 0> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 0> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<int> s = {a, 1};
		CHECK_SPAN_1(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, 2};
		CHECK_SPAN_2(s, a);
	}

	SECTION("")
	{
		utility::span<int, 3> s = {a, 3};
		CHECK_SPAN_3(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 4> s = {a, 4};
		CHECK_SPAN_4(s, a);
	}

	SECTION("")
	{
		utility::span<int> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<int, 0> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 0> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<int> s = {a, a + 1};
		CHECK_SPAN_1(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, a + 2};
		CHECK_SPAN_2(s, a);
	}

	SECTION("")
	{
		utility::span<int, 3> s = {a, a + 3};
		CHECK_SPAN_3(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 4> s = {a, a + 4};
		CHECK_SPAN_4(s, a);
	}

	SECTION("")
	{
		utility::span<int> s = a;
		CHECK_SPAN_5(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = a;
		CHECK_SPAN_5(s, a);
	}

	SECTION("")
	{
		utility::span<int, 5> s = a;
		CHECK_SPAN_5(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 5> s = a;
		CHECK_SPAN_5(s, a);
	}
}

TEST_CASE("", "[utility][span]")
{
	const int a[] = {1, 2, 3, 4, 5};

	SECTION("")
	{
		utility::span<const int> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 0> s = {a, 0};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, 2};
		CHECK_SPAN_2(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 4> s = {a, 4};
		CHECK_SPAN_4(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 0> s = {a, a};
		CHECK_SPAN_0(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = {a, a + 2};
		CHECK_SPAN_2(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 4> s = {a, a + 4};
		CHECK_SPAN_4(s, a);
	}

	SECTION("")
	{
		utility::span<const int> s = a;
		CHECK_SPAN_5(s, a);
	}

	SECTION("")
	{
		utility::span<const int, 5> s = a;
		CHECK_SPAN_5(s, a);
	}
}

TEST_CASE("", "[utility][span]")
{
	std::array<int, 5> a = {1, 2, 3, 4, 5};

	SECTION("")
	{
		utility::span<int> s = a;
		CHECK_SPAN_5(s, a.data());
	}

	SECTION("")
	{
		utility::span<const int> s = a;
		CHECK_SPAN_5(s, a.data());
	}

	SECTION("")
	{
		utility::span<int, 5> s = a;
		CHECK_SPAN_5(s, a.data());
	}

	SECTION("")
	{
		utility::span<const int, 5> s = a;
		CHECK_SPAN_5(s, a.data());
	}
}

TEST_CASE("", "[utility][span]")
{
	const std::array<int, 5> a = {1, 2, 3, 4, 5};

	SECTION("")
	{
		utility::span<const int> s = a;
		CHECK_SPAN_5(s, a.data());
	}

	SECTION("")
	{
		utility::span<const int, 5> s = a;
		CHECK_SPAN_5(s, a.data());
	}
}

TEST_CASE("", "[utility][span]")
{
	int a[] = {1, 2, 3, 4, 5};

	SECTION("")
	{
		utility::span<int> s = a;

		SECTION("")
		{
			utility::span<int> ss = s;
			CHECK_SPAN_5(ss, a);
		}

		SECTION("")
		{
			utility::span<const int> ss = s;
			CHECK_SPAN_5(ss, a);
		}
	}

	SECTION("")
	{
		utility::span<const int> s = a;

		SECTION("")
		{
			utility::span<const int> ss = s;
			CHECK_SPAN_5(ss, a);
		}
	}

	SECTION("")
	{
		utility::span<int, 5> s = a;

		SECTION("")
		{
			utility::span<int> ss = s;
			CHECK_SPAN_5(ss, a);
		}

		SECTION("")
		{
			utility::span<const int> ss = s;
			CHECK_SPAN_5(ss, a);
		}

		SECTION("")
		{
			utility::span<int, 5> ss = s;
			CHECK_SPAN_5(ss, a);
		}

		SECTION("")
		{
			utility::span<const int, 5> ss = s;
			CHECK_SPAN_5(ss, a);
		}
	}

	SECTION("")
	{
		utility::span<const int, 5> s = a;

		SECTION("")
		{
			utility::span<const int> ss = s;
			CHECK_SPAN_5(ss, a);
		}

		SECTION("")
		{
			utility::span<const int, 5> ss = s;
			CHECK_SPAN_5(ss, a);
		}
	}
}

TEST_CASE("utility span functions", "[utility][span]")
{
	int a[] = {1, 2, 3};

	SECTION("for dynamic spans")
	{
		utility::span<int> s = a;

		SECTION("supports empty")
		{
			CHECK(!utility::empty(s));

			CHECK(utility::empty(utility::span<int>(a, 0)));
		}

		SECTION("supports front")
		{
			int & i = utility::front(s);
			CHECK(&i == a);
		}

		SECTION("supports back")
		{
			int & i = utility::back(s);
			CHECK(&i == a + 2);
		}

		SECTION("supports static first")
		{
			utility::span<int, 0> ss = utility::first<0>(s);
			CHECK(ss.data() == a);

			utility::span<int, 3> sss = utility::first<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic first")
		{
			utility::span<int> ss = utility::first(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::first(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("supports static last")
		{
			utility::span<int, 0> ss = utility::last<0>(s);
			CHECK(ss.data() == a + 3);

			utility::span<int, 3> sss = utility::last<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic last")
		{
			utility::span<int> ss = utility::last(s, 0);
			CHECK(ss.data() == a + 3);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::last(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan<0>(s);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<int> sss = utility::subspan<3>(s);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<int, 0> ss = utility::subspan<0, 0>(s);
			CHECK(ss.data() == a);

			utility::span<int, 3> sss = utility::subspan<0, 3>(s);
			CHECK(sss.data() == a);

			utility::span<int, 0> ssss = utility::subspan<3, 0>(s);
			CHECK(ssss.data() == a + 3);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<int> sss = utility::subspan(s, 3);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan(s, 0, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::subspan(s, 0, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);

			utility::span<int> ssss = utility::subspan(s, 3, 0);
			CHECK(ssss.data() == a + 3);
			CHECK(ssss.size() == 0);
		}
	}

	SECTION("for static spans")
	{
		utility::span<int, 3> s = a;

		SECTION("supports empty")
		{
			CHECK(!utility::empty(s));

			CHECK(utility::empty(utility::span<int, 0>(a, 0)));
		}

		SECTION("supports front")
		{
			int & i = utility::front(s);
			CHECK(&i == a);
		}

		SECTION("supports back")
		{
			int & i = utility::back(s);
			CHECK(&i == a + 2);
		}

		SECTION("supports static first")
		{
			utility::span<int, 0> ss = utility::first<0>(s);
			CHECK(ss.data() == a);

			utility::span<int, 3> sss = utility::first<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic first")
		{
			utility::span<int> ss = utility::first(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::first(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("supports static last")
		{
			utility::span<int, 0> ss = utility::last<0>(s);
			CHECK(ss.data() == a + 3);

			utility::span<int, 3> sss = utility::last<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic last")
		{
			utility::span<int> ss = utility::last(s, 0);
			CHECK(ss.data() == a + 3);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::last(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan<0>(s);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<int> sss = utility::subspan<3>(s);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<int, 0> ss = utility::subspan<0, 0>(s);
			CHECK(ss.data() == a);

			utility::span<int, 3> sss = utility::subspan<0, 3>(s);
			CHECK(sss.data() == a);

			utility::span<int, 0> ssss = utility::subspan<3, 0>(s);
			CHECK(ssss.data() == a + 3);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<int> sss = utility::subspan(s, 3);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<int> ss = utility::subspan(s, 0, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<int> sss = utility::subspan(s, 0, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);

			utility::span<int> ssss = utility::subspan(s, 3, 0);
			CHECK(ssss.data() == a + 3);
			CHECK(ssss.size() == 0);
		}
	}

	SECTION("for dynamic const spans")
	{
		utility::span<const int> s = a;

		SECTION("supports empty")
		{
			CHECK(!utility::empty(s));

			CHECK(utility::empty(utility::span<const int>(a, 0)));
		}

		SECTION("supports front")
		{
			const int & i = utility::front(s);
			CHECK(&i == a);
		}

		SECTION("supports back")
		{
			const int & i = utility::back(s);
			CHECK(&i == a + 2);
		}

		SECTION("supports static first")
		{
			utility::span<const int, 0> ss = utility::first<0>(s);
			CHECK(ss.data() == a);

			utility::span<const int, 3> sss = utility::first<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic first")
		{
			utility::span<const int> ss = utility::first(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::first(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("supports static last")
		{
			utility::span<const int, 0> ss = utility::last<0>(s);
			CHECK(ss.data() == a + 3);

			utility::span<const int, 3> sss = utility::last<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic last")
		{
			utility::span<const int> ss = utility::last(s, 0);
			CHECK(ss.data() == a + 3);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::last(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan<0>(s);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<const int> sss = utility::subspan<3>(s);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<const int, 0> ss = utility::subspan<0, 0>(s);
			CHECK(ss.data() == a);

			utility::span<const int, 3> sss = utility::subspan<0, 3>(s);
			CHECK(sss.data() == a);

			utility::span<const int, 0> ssss = utility::subspan<3, 0>(s);
			CHECK(ssss.data() == a + 3);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<const int> sss = utility::subspan(s, 3);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan(s, 0, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::subspan(s, 0, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);

			utility::span<const int> ssss = utility::subspan(s, 3, 0);
			CHECK(ssss.data() == a + 3);
			CHECK(ssss.size() == 0);
		}
	}

	SECTION("for static const spans")
	{
		utility::span<const int, 3> s = a;

		SECTION("supports empty")
		{
			CHECK(!utility::empty(s));

			CHECK(utility::empty(utility::span<const int, 0>(a, 0)));
		}

		SECTION("supports front")
		{
			const int & i = utility::front(s);
			CHECK(&i == a);
		}

		SECTION("supports back")
		{
			const int & i = utility::back(s);
			CHECK(&i == a + 2);
		}

		SECTION("supports static first")
		{
			utility::span<const int, 0> ss = utility::first<0>(s);
			CHECK(ss.data() == a);

			utility::span<const int, 3> sss = utility::first<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic first")
		{
			utility::span<const int> ss = utility::first(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::first(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("supports static last")
		{
			utility::span<const int, 0> ss = utility::last<0>(s);
			CHECK(ss.data() == a + 3);

			utility::span<const int, 3> sss = utility::last<3>(s);
			CHECK(sss.data() == a);
		}

		SECTION("supports dynamic last")
		{
			utility::span<const int> ss = utility::last(s, 0);
			CHECK(ss.data() == a + 3);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::last(s, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan<0>(s);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<const int> sss = utility::subspan<3>(s);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<const int, 0> ss = utility::subspan<0, 0>(s);
			CHECK(ss.data() == a);

			utility::span<const int, 3> sss = utility::subspan<0, 3>(s);
			CHECK(sss.data() == a);

			utility::span<const int, 0> ssss = utility::subspan<3, 0>(s);
			CHECK(ssss.data() == a + 3);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan(s, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 3);

			utility::span<const int> sss = utility::subspan(s, 3);
			CHECK(sss.data() == a + 3);
			CHECK(sss.size() == 0);
		}

		SECTION("")
		{
			utility::span<const int> ss = utility::subspan(s, 0, 0);
			CHECK(ss.data() == a);
			CHECK(ss.size() == 0);

			utility::span<const int> sss = utility::subspan(s, 0, 3);
			CHECK(sss.data() == a);
			CHECK(sss.size() == 3);

			utility::span<const int> ssss = utility::subspan(s, 3, 0);
			CHECK(ssss.data() == a + 3);
			CHECK(ssss.size() == 0);
		}
	}
}
