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
		return utility::make_lookup_table(
			std::make_pair(utility::string_units_utf8("zero"), Enum::zero),
			std::make_pair(utility::string_units_utf8("one"), Enum::one),
			std::make_pair(utility::string_units_utf8("two"), Enum::two),
			std::make_pair(utility::string_units_utf8("three"), Enum::three),
			std::make_pair(utility::string_units_utf8("four"), Enum::four),
			std::make_pair(utility::string_units_utf8("five"), Enum::five)
			);
	}
}

TEST_CASE("", "[core][ini][structurer]")
{
	const char simple[] = u8R"(
; variables without group
Int=79817
Bool=1
; variables with group
[group1]
type=one
Int=1234
String=a long string with many spaces without quotes because that is not needed
[group2]
type=two
Int=-789872
String=name
)";

	struct stream_data
	{
		const char * begin;
		const char * end;
	}
	stream_data{simple, simple + sizeof simple - 1};

	core::IniStructurer structurer(core::ReadStream(
		[](void * dest, ext::usize n, void * data) -> ext::ssize
		{
			auto & d = *static_cast<struct stream_data *>(data);
			n = std::min(n, ext::usize(d.end - d.begin));
			std::copy_n(d.begin, n, static_cast<char *>(dest));
			d.begin += n;
			return n;
		},
		&stream_data,
		u8""));

	SECTION("can be read into empty struct")
	{
		struct data_type
		{
			int dummy_;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(std::make_pair(utility::string_units_utf8("dummy"), &data_type::dummy_));
			}
		}
		data;

		REQUIRE(structurer.read(data));
	}

	SECTION("can be read into full struct")
	{
		struct data_type
		{
			struct group1_type
			{
				test::Enum type;
				int Int;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("type"), &group1_type::type),
						std::make_pair(utility::string_units_utf8("Int"), &group1_type::Int)
					);
				}
			}
			group1;

			struct group2_type
			{
				test::Enum type;
				int Int;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("type"), &group2_type::type),
						std::make_pair(utility::string_units_utf8("Int"), &group2_type::Int)
					);
				}
			}
			group2;

			int Int;
			bool Bool;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("Int"), &data_type::Int),
					std::make_pair(utility::string_units_utf8("Bool"), &data_type::Bool),
					std::make_pair(utility::string_units_utf8("group1"), &data_type::group1),
					std::make_pair(utility::string_units_utf8("group2"), &data_type::group2)
					);
			}
		}
		data;

		REQUIRE(structurer.read(data));
	}
}
