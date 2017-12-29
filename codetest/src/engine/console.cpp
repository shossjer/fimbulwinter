
#include "catch.hpp"

#include "engine/console.hpp"

namespace engine
{
	namespace console
	{
		extern std::vector<Param> parse_params(const std::string & raw_line);
		extern bool is_number_candidate(const std::string & str);
	}
}

using namespace engine::console;

namespace
{
	template<typename T>
	T test(std::string str_param, T expected)
	{
		auto params = parse_params(str_param);
		REQUIRE(params.size() == 1);
		REQUIRE(utility::holds_alternative<T>(params[0]));

		T actual = utility::get<T>(params[0]);
		REQUIRE(actual == expected);
		return actual;
	}
}

TEST_CASE("Verify is_number_candidate method")
{
	REQUIRE(is_number_candidate("0"));
	REQUIRE(is_number_candidate("-0"));
	REQUIRE(is_number_candidate("0.1"));
	REQUIRE(is_number_candidate("1000."));

	REQUIRE(!is_number_candidate("0xFF"));
	REQUIRE(!is_number_candidate("0.1f"));
	REQUIRE(!is_number_candidate("zero"));
	REQUIRE(!is_number_candidate("true"));
	REQUIRE(!is_number_candidate(" "));
}

TEST_CASE("Verify boolean parsing")
{
	SECTION("Parse booleans")
	{
		test("false", false);
		test("true", true);
	}
}

TEST_CASE("Verify number parsing")
{
	SECTION("Signed Integer")
	{
		test("-1", -1);
		test("-1234", -1234);
		test("-0", -0);
	}
	SECTION("Unsigned Integer")
	{
		test("1", 1);
		test("1234", 1234);
		test("0", 0);
	}
	SECTION("Float")
	{
		test("0.0", 0.0f);
		test("1.0", 1.0f);
		test("1234.5678", 1234.5678f);
	}
}

TEST_CASE("Verify string parsing")
{
	SECTION("Parse strings")
	{
		test<std::string>("a_single_string", "a_single_string");
	}
	SECTION("Parse string scope")
	{
		test<std::string>("\"String with spaces\"", "String with spaces");
	}
	SECTION("Non-strings as strings")
	{
		test<std::string>("\"1234\"", "1234");
		test<std::string>("\"0.5\"", "0.5");
		test<std::string>("\"false\"", "false");
		test<std::string>("\"true\"", "true");
	}
	SECTION("Number-like strings")
	{
		test<std::string>("0x1234", "0x1234");
		test<std::string>("0xffff", "0xffff");
		test<std::string>("12b", "12b");
		test<std::string>("a1234", "a1234");
	}
}

TEST_CASE("Verify multiple args")
{
	SECTION("Parse strings")
	{
		auto params = parse_params("\"String with spaces\" hey_how_are_you \"Another string\" \"true\"");

		REQUIRE(params.size() == 4);
		REQUIRE(utility::holds_alternative<std::string>(params[0]));
		REQUIRE(utility::holds_alternative<std::string>(params[1]));
		REQUIRE(utility::holds_alternative<std::string>(params[2]));
		REQUIRE(utility::holds_alternative<std::string>(params[3]));

		REQUIRE(utility::get<std::string>(params[0]) == "String with spaces");
		REQUIRE(utility::get<std::string>(params[1]) == "hey_how_are_you");
		REQUIRE(utility::get<std::string>(params[2]) == "Another string");
		REQUIRE(utility::get<std::string>(params[3]) == "true");
	}
	SECTION("Parse mix")
	{
		auto params = parse_params("\"String with spaces\" a_string true 1234 0.5");

		REQUIRE(params.size() == 5);
		REQUIRE(utility::holds_alternative<std::string>(params[0]));
		REQUIRE(utility::holds_alternative<std::string>(params[1]));
		REQUIRE(utility::holds_alternative<bool>(params[2]));
		REQUIRE(utility::holds_alternative<int>(params[3]));
		REQUIRE(utility::holds_alternative<float>(params[4]));

		REQUIRE(utility::get<std::string>(params[0]) == "String with spaces");
		REQUIRE(utility::get<std::string>(params[1]) == "a_string");
		REQUIRE(utility::get<bool>(params[2]) == true);
		REQUIRE(utility::get<int>(params[3]) == 1234);
		REQUIRE(utility::get<float>(params[4]) == 0.5f);
	}
}

namespace
{
	void f_empty() {}
	void f_single(std::string) {}
	void f_dual1(bool, int) {}
	void f_dual2(bool, bool) {}
}

// TODO: test variadic lookup
TEST_CASE("Verify variadic parsing")
{
	SECTION("Lookup no args")
	{
		Callback<> callback{ f_empty };
		std::vector<Param> params;
		REQUIRE(callback.call(params));
		params.emplace_back(utility::in_place_type<std::string>, "");
		REQUIRE(!callback.call(params));
	}

	SECTION("Lookup args")
	{
		Callback<> c_empty{ f_empty };
		Callback<std::string> c_single{ f_single };

		std::vector<Param> params;
		REQUIRE(c_empty.call(params));		// match
		REQUIRE(!c_single.call(params));	// too few args

		params.emplace_back(utility::in_place_type<std::string>, "");

		REQUIRE(!c_empty.call(params));		// too many args
		REQUIRE(c_single.call(params));		// match
	}

	SECTION("Lookup argument missmatch")
	{
		Callback<std::string> c_single{ f_single };

		std::vector<Param> params;
		params.emplace_back(utility::in_place_type<bool>, false);

		REQUIRE(!c_single.call(params));	// missmatch
	}
	SECTION("Lookup multi args match")
	{
		std::vector<Param> params;

		Callback<bool, int> call_match{ f_dual1 };
		Callback<bool, bool> call_inval{ f_dual2 };

		params.emplace_back(utility::in_place_type<bool>, false);
		REQUIRE(!call_match.call(params));	// too few args
		REQUIRE(!call_inval.call(params));	// too few args

		params.emplace_back(utility::in_place_type<int>, 0);
		REQUIRE(call_match.call(params));	// match
		REQUIRE(!call_inval.call(params));	// missmatch
	}
}
