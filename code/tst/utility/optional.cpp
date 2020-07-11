#include "utility/optional.hpp"

#include <catch2/catch.hpp>

#include <array>

TEST_CASE( "optional", "[utility]" )
{
	// auto myarray = std::array<int, 7>{{1, 2, 3, 4, 5, 6, 7}};
	// auto myarrayspan = utility::make_array_span(myarray);
	// (void)myarrayspan; // fix warning about not being used
	// // auto myhalfspan = make_array_span<3>(myarrayspan.begin() + 2);

	// // iterator_traits:
	// // * access the underlaying type?

	// auto kjhs = utility::array_span<int, 3>{myarray.begin() + 2};
	// (void)kjhs; // fix warning about not being used

	constexpr utility::optional<int> inta;
	static_assert(!inta.has_value(), "");
	constexpr utility::optional<int> intb{};
	static_assert(!intb.has_value(), "");
	constexpr utility::optional<int> intc{1};
	static_assert(intc.has_value(), "");
	static_assert(*intc == 1, "");

	constexpr utility::optional<std::array<int, 4>> intd{utility::in_place};
	static_assert(intd->size() == 4, "");
}
