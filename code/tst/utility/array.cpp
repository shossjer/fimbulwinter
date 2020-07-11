#include "utility/array.hpp"

#include <catch2/catch.hpp>

#include <array>

TEST_CASE( "array_span", "[utility]" )
{
	auto myarray = std::array<int, 7>{{1, 2, 3, 4, 5, 6, 7}};
	auto myarrayspan = utility::make_array_span(myarray);
	(void)myarrayspan; // fix warning about not being used
	// auto myhalfspan = make_array_span<3>(myarrayspan.begin() + 2);

	// iterator_traits:
	// * access the underlaying type?

	//auto kjhs = utility::array_span<int, 3>{myarray.begin() + 2};
	//(void)kjhs; // fix warning about not being used
}
