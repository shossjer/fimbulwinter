#include "core/color.hpp"
#include "core/container/Buffer.hpp"
#include "core/JsonStructurer.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include <catch2/catch.hpp>

namespace
{
	enum class Enum
	{
		zero,
		one,
		two,
		three,
		four,
		five,
		six,
		seven,
	};

	constexpr auto serialization(utility::in_place_type_t<Enum>)
	{
		return utility::make_lookup_table<ful::view_utf8>(
			std::make_pair(ful::cstr_utf8("zero"), Enum::zero),
			std::make_pair(ful::cstr_utf8("one"), Enum::one),
			std::make_pair(ful::cstr_utf8("two"), Enum::two),
			std::make_pair(ful::cstr_utf8("three"), Enum::three),
			std::make_pair(ful::cstr_utf8("four"), Enum::four),
			std::make_pair(ful::cstr_utf8("five"), Enum::five),
			std::make_pair(ful::cstr_utf8("six"), Enum::five),
			std::make_pair(ful::cstr_utf8("seven"), Enum::five)
			);
	}
}

TEST_CASE("integer json arrays", "[core][json][structurer]")
{
	char integer_array[] = u8R"(
[ 9, -8, 7, -6, 5, -4, 3, -2, 1 ]
)";

	core::content content(ful::cstr_utf8(""), integer_array, sizeof integer_array - 1);
	core::JsonStructurer structurer(content);

	SECTION("can be read into c-array")
	{
		int data[9];

		REQUIRE(structurer.read(data));
		CHECK(data[0] == 9);
		CHECK(data[1] == -8);
		CHECK(data[2] == 7);
		CHECK(data[3] == -6);
		CHECK(data[4] == 5);
		CHECK(data[5] == -4);
		CHECK(data[6] == 3);
		CHECK(data[7] == -2);
		CHECK(data[8] == 1);
	}

	SECTION("can be read into std::array")
	{
		std::array<std::int8_t, 9> data;

		REQUIRE(structurer.read(data));
		CHECK(data[0] == 9);
		CHECK(data[1] == -8);
		CHECK(data[2] == 7);
		CHECK(data[3] == -6);
		CHECK(data[4] == 5);
		CHECK(data[5] == -4);
		CHECK(data[6] == 3);
		CHECK(data[7] == -2);
		CHECK(data[8] == 1);
	}

	SECTION("can be read into std::tuple")
	{
		std::tuple<std::uint8_t, std::int8_t, std::uint8_t, std::int8_t, std::uint8_t, std::int8_t, std::uint8_t, std::int8_t, std::uint8_t> data;

		REQUIRE(structurer.read(data));
		CHECK(std::get<0>(data) == 9);
		CHECK(std::get<1>(data) == -8);
		CHECK(std::get<2>(data) == 7);
		CHECK(std::get<3>(data) == -6);
		CHECK(std::get<4>(data) == 5);
		CHECK(std::get<5>(data) == -4);
		CHECK(std::get<6>(data) == 3);
		CHECK(std::get<7>(data) == -2);
		CHECK(std::get<8>(data) == 1);
	}

	SECTION("can be read into std::vector")
	{
		std::vector<float> data;

		REQUIRE(structurer.read(data));
		REQUIRE(data.size() == 9);
		CHECK(data[0] == 9.f);
		CHECK(data[1] == -8.f);
		CHECK(data[2] == 7.f);
		CHECK(data[3] == -6.f);
		CHECK(data[4] == 5.f);
		CHECK(data[5] == -4.f);
		CHECK(data[6] == 3.f);
		CHECK(data[7] == -2.f);
		CHECK(data[8] == 1.f);
	}

	SECTION("can be read into core::container::Buffer")
	{
		core::container::Buffer data;

		REQUIRE(structurer.read(data));
		REQUIRE(data.size() == 9);
		REQUIRE(data.value_type() == utility::type_id<int8_t>());
		CHECK(data.data_as<int8_t>()[0] == 9);
		CHECK(data.data_as<int8_t>()[1] == -8);
		CHECK(data.data_as<int8_t>()[2] == 7);
		CHECK(data.data_as<int8_t>()[3] == -6);
		CHECK(data.data_as<int8_t>()[4] == 5);
		CHECK(data.data_as<int8_t>()[5] == -4);
		CHECK(data.data_as<int8_t>()[6] == 3);
		CHECK(data.data_as<int8_t>()[7] == -2);
		CHECK(data.data_as<int8_t>()[8] == 1);
	}
}

TEST_CASE("simple json objects", "[core][json][structurer]")
{
	char simple_object[] = u8R"(
{
	"boolean": true,
	"enum name": "three",
	"numbers":
	{
		"floating point": 0.25,
		"signed integer": -1,
		"unsigned integer": 111
	}
}
)";

	core::content content(ful::cstr_utf8(""), simple_object, sizeof simple_object - 1);
	core::JsonStructurer structurer(content);

	SECTION("can be read into full struct")
	{
		struct data_type
		{
			struct number_type
			{
				float floating_point;
				int signed_integer;
				unsigned int unsigned_integer;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table<ful::view_utf8>(
						std::make_pair(ful::cstr_utf8("floating point"), &number_type::floating_point),
						std::make_pair(ful::cstr_utf8("signed integer"), &number_type::signed_integer),
						std::make_pair(ful::cstr_utf8("unsigned integer"), &number_type::unsigned_integer)
					);
				}
			}
			number;

			bool boolean;
			Enum enum_name;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("boolean"), &data_type::boolean),
					std::make_pair(ful::cstr_utf8("enum name"), &data_type::enum_name),
					std::make_pair(ful::cstr_utf8("numbers"), &data_type::number)
					);
			}
		}
		data{};

		REQUIRE(structurer.read(data));
		CHECK(data.boolean);
		CHECK(data.enum_name == Enum::three);
		CHECK(data.number.floating_point == .25f);
		CHECK(data.number.signed_integer == -1);
		CHECK(data.number.unsigned_integer == 111);
	}
}

TEST_CASE("", "[core][json][structurer]")
{
	char color_source[] = u8R"(
{
	"rgb type":
	{
		"r": 255,
		"g": 0,
		"b": 127
	},
	"rgb array":
	[
		0.5,
		1.0,
		0.0
	]
}
)";

	core::content content(ful::cstr_utf8(""), color_source, sizeof color_source - 1);
	core::JsonStructurer structurer(content);

	SECTION("can be read into rgb type")
	{
		struct data_type
		{
			core::rgb_t<std::uint8_t> rgb_as_class;
			core::rgb_t<float> rgb_as_array;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("rgb type"), &data_type::rgb_as_class),
					std::make_pair(ful::cstr_utf8("rgb array"), &data_type::rgb_as_array)
				);
			}
		}
		data{};

		REQUIRE(structurer.read(data));
		CHECK(data.rgb_as_class.red() == 255);
		CHECK(data.rgb_as_class.green() == 0);
		CHECK(data.rgb_as_class.blue() == 127);
		CHECK(data.rgb_as_array.red() == .5f);
		CHECK(data.rgb_as_array.green() == 1.f);
		CHECK(data.rgb_as_array.blue() == 0.f);
	}
}

TEST_CASE("", "[core][json][structurer]")
{
	char vector_source[] = u8R"(
{
	"four floats": [ 0.25, 0.75, -2.0, -1.5 ]
}
)";

	core::content content(ful::cstr_utf8(""), vector_source, sizeof vector_source - 1);
	core::JsonStructurer structurer(content);

	SECTION("can be read into vector type")
	{
		struct data_type
		{
			core::maths::Vector4f vector;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("four floats"), &data_type::vector)
				);
			}
		}
		data{};

		REQUIRE(structurer.read(data));
		typename core::maths::Vector4f::array_type buffer;
		data.vector.get(buffer);
		CHECK(buffer[0] == 0.25);
		CHECK(buffer[1] == 0.75);
		CHECK(buffer[2] == -2.0);
		CHECK(buffer[3] == -1.5);
	}

	SECTION("can be read into quaternion type")
	{
		struct data_type
		{
			core::maths::Quaternionf quat;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("four floats"), &data_type::quat)
				);
			}
		}
		data{};

		REQUIRE(structurer.read(data));
		typename core::maths::Quaternionf::array_type buffer;
		data.quat.get(buffer);
		CHECK(buffer[0] == 0.25);
		CHECK(buffer[1] == 0.75);
		CHECK(buffer[2] == -2.0);
		CHECK(buffer[3] == -1.5);
	}
}
