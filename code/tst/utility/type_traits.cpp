#include <catch.hpp>

#include "utility/type_traits.hpp"

TEST_CASE("common_type_unless_nonvoid", "[utility][type_traits]")
{
	static_assert(std::is_same<int, mpl::common_type_unless_nonvoid<int>>::value, "");
	static_assert(std::is_same<int, mpl::common_type_unless_nonvoid<int, float>>::value, "");
	static_assert(std::is_same<int, mpl::common_type_unless_nonvoid<int, float, char, int>>::value, "");

	static_assert(std::is_same<float, mpl::common_type_unless_nonvoid<void, float>>::value, "");
	static_assert(std::is_same<float, mpl::common_type_unless_nonvoid<void, float, char, int>>::value, "");
}
