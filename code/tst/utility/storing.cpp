#include "utility/storing.hpp"

#include <catch/catch.hpp>

namespace
{
	struct not_default_constructible
	{
		not_default_constructible(int) {}
	};

	struct also_not_default_constructible
	{
		int & ref;
	};

	static_assert(!std::is_default_constructible<not_default_constructible>::value, "");
	static_assert(!std::is_trivial<not_default_constructible>::value, "");
	static_assert(!std::is_default_constructible<also_not_default_constructible>::value, "");
#if defined(__GNUG__)
	static_assert(std::is_trivial<also_not_default_constructible>::value, ""); // wtf!?
#elif defined(_MSC_VER)
	static_assert(!std::is_trivial<also_not_default_constructible>::value, ""); // this makes more sense! but how can clang be wrong :fearful:
#endif
}

TEST_CASE("", "")
{
	utility::storing<not_default_constructible> a;
}

TEST_CASE("", "")
{
	utility::storing<also_not_default_constructible> a;
}
