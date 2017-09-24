
#include "catch.hpp"

#include <core/debug.hpp>

namespace
{
	struct A
	{
		int i;

		A() = delete;
		A(int i) : i(i) {}
		A(const A &) = delete;
		A(A &&) = delete;

		friend bool operator == (const A & a1, const A & a2)
		{
			return a1.i == a2.i;
		}
	};
}

TEST_CASE( "debug_assert", "[utility]" )
{
	debug_assert(1 + 1 == 2);
	debug_assert(1 < 2);

	// Visual Studio say nope
	//debug_assert(std::string("hola") == "hola");

	debug_assert(A{1} == A{1});

	debug_assert(true);
	debug_assert((3 == 3));
	debug_assert(nullptr == nullptr);

	const int a = 7;
	const int b = 7;
	debug_assert(a == b);

	debug_assert(1 != 2);
	debug_assert(a < 11);
	debug_assert(b <= 7);
	debug_assert(a > 3);
	debug_assert(8 >= 7);
}
