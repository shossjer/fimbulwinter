#include "core/serialization.hpp"

#include <catch2/catch.hpp>

#include <string>

TEST_CASE("", "[core][serialization]")
{
	struct S
	{
		int a;
		float b;
		std::string c;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("avar"), &S::a),
				std::make_pair(utility::string_units_utf8("bfoo"), &S::b),
				std::make_pair(utility::string_units_utf8("cbar"), &S::c)
				);
		}
	};

	CHECK(core::member_table<S>::find("avar") == 0);
	CHECK(core::member_table<S>::find("bfoo") == 1);
	CHECK(core::member_table<S>::find("cbar") == 2);
	CHECK(core::member_table<S>::find("ntho") == ext::index_invalid);
}
