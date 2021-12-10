#include "core/serialization.hpp"

#include "utility/ext/stddef.hpp"

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
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("avar"), &S::a),
				std::make_pair(ful::cstr_utf8("bfoo"), &S::b),
				std::make_pair(ful::cstr_utf8("cbar"), &S::c)
				);
		}
	};

	CHECK(core::member_table<S>::find(ful::cstr_utf8("avar")) == 0);
	CHECK(core::member_table<S>::find(ful::cstr_utf8("bfoo")) == 1);
	CHECK(core::member_table<S>::find(ful::cstr_utf8("cbar")) == 2);
	CHECK(core::member_table<S>::find(ful::cstr_utf8("ntho")) == ext::index_invalid);
}
