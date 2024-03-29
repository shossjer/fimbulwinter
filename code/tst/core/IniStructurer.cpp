#include "core/IniStructurer.hpp"

#include <catch2/catch.hpp>

namespace test
{
	enum class Enum
	{
		zero,
		one,
		two,
		three,
		four,
		five,
	};

	constexpr auto serialization(utility::in_place_type_t<Enum>)
	{
		return utility::make_lookup_table<ful::view_utf8>(
			std::make_pair(ful::cstr_utf8("zero"), Enum::zero),
			std::make_pair(ful::cstr_utf8("one"), Enum::one),
			std::make_pair(ful::cstr_utf8("two"), Enum::two),
			std::make_pair(ful::cstr_utf8("three"), Enum::three),
			std::make_pair(ful::cstr_utf8("four"), Enum::four),
			std::make_pair(ful::cstr_utf8("five"), Enum::five)
			);
	}
}

TEST_CASE("", "[core][ini][structurer]")
{
	char simple[] = R"(
; variables without group
Int = 79817
Bool = true
; variables with group
[group1]
type = one
Int = 1234
String = a long string with many spaces without quotes because that is not needed
[group2]
type = two
Int = -789872
String = name
)";

	core::content content(ful::cstr_utf8(""), simple + 0, sizeof simple - 1);

	SECTION("can be read into full struct")
	{
		struct data_type
		{
			struct group1_type
			{
				test::Enum type;
				int Int;
				ful::view_utf8 String;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table<ful::view_utf8>(
						std::make_pair(ful::cstr_utf8("type"), &group1_type::type),
						std::make_pair(ful::cstr_utf8("Int"), &group1_type::Int),
						std::make_pair(ful::cstr_utf8("String"), &group1_type::String)
					);
				}
			}
			group1;

			struct group2_type
			{
				test::Enum type;
				int Int;
				ful::view_utf8 String;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table<ful::view_utf8>(
						std::make_pair(ful::cstr_utf8("type"), &group2_type::type),
						std::make_pair(ful::cstr_utf8("Int"), &group2_type::Int),
						std::make_pair(ful::cstr_utf8("String"), &group2_type::String)
					);
				}
			}
			group2;

			int Int;
			bool Bool;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("Int"), &data_type::Int),
					std::make_pair(ful::cstr_utf8("Bool"), &data_type::Bool),
					std::make_pair(ful::cstr_utf8("group1"), &data_type::group1),
					std::make_pair(ful::cstr_utf8("group2"), &data_type::group2)
					);
			}
		}
		data{};

		REQUIRE(core::structure_ini(content, data));
		CHECK(data.Int == 79817);
		CHECK(data.Bool == true);
		CHECK(data.group1.type == test::Enum::one);
		CHECK(data.group1.Int == 1234);
		CHECK(data.group1.String == "a long string with many spaces without quotes because that is not needed");
		CHECK(data.group2.type == test::Enum::two);
		CHECK(data.group2.Int == -789872);
		CHECK(data.group2.String == "name");
	}
}
