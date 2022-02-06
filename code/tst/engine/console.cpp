#include "engine/console.hpp"

#include "ful/cstr.hpp"
#include "ful/string_compare.hpp"

#include <catch2/catch.hpp>

namespace engine
{
	namespace detail
	{
		extern utility::heap_vector<Argument> parse_params(ful::view_utf8 line);
	}
}

#define test(line, expected) \
	do \
	{ \
		const auto params = engine::detail::parse_params(ful::cstr_utf8(line)); \
		REQUIRE(params.size() == 1); \
		REQUIRE(utility::holds_alternative<decltype(expected)>(params[0])); \
		REQUIRE(utility::get<decltype(expected)>(params[0]) == expected); \
	} \
	while (false)

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
		test("-1", int64_t(-1));
		test("-1234", int64_t(-1234));
		test("-0", int64_t(-0));
	}
	SECTION("Unsigned Integer")
	{
		test("1", int64_t(1));
		test("1234", int64_t(1234));
		test("0", int64_t(0));
	}
	SECTION("Float")
	{
		test("0.0", 0.0);
		test("-1.0", -1.0);
		test("1234.5678", static_cast<double>(1234.5678f));
	}
}

TEST_CASE("Verify string parsing")
{
	SECTION("Parse strings")
	{
		test("\"a_single_string\"", static_cast<ful::view_utf8>(ful::cstr_utf8("a_single_string")));
	}
	SECTION("Parse string scope")
	{
		test("\"String with spaces\"", static_cast<ful::view_utf8>(ful::cstr_utf8("String with spaces")));
	}
	SECTION("Non-strings as strings")
	{
		test("\"1234\"", static_cast<ful::view_utf8>(ful::cstr_utf8("1234")));
		test("\"0.5\"", static_cast<ful::view_utf8>(ful::cstr_utf8("0.5")));
		test("\"false\"", static_cast<ful::view_utf8>(ful::cstr_utf8("false")));
		test("\"true\"", static_cast<ful::view_utf8>(ful::cstr_utf8("true")));
	}
	SECTION("Number-like strings")
	{
		test("\"0x1234\"", static_cast<ful::view_utf8>(ful::cstr_utf8("0x1234")));
		test("\"0xffff\"", static_cast<ful::view_utf8>(ful::cstr_utf8("0xffff")));
		test("\"12b\"", static_cast<ful::view_utf8>(ful::cstr_utf8("12b")));
		test("\"a1234\"", static_cast<ful::view_utf8>(ful::cstr_utf8("a1234")));
	}
}

TEST_CASE("Verify multiple args")
{
	SECTION("Parse strings")
	{
		const ful::cstr_utf8 line("\"String with spaces\" \"hey_how_are_you\" \"Another string\" \"true\"");
		const auto params = engine::detail::parse_params(line);

		REQUIRE(params.size() == 4);
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[0]));
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[1]));
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[2]));
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[3]));

		REQUIRE(utility::get<ful::view_utf8>(params[0]) == "String with spaces");
		REQUIRE(utility::get<ful::view_utf8>(params[1]) == "hey_how_are_you");
		REQUIRE(utility::get<ful::view_utf8>(params[2]) == "Another string");
		REQUIRE(utility::get<ful::view_utf8>(params[3]) == "true");
	}
	SECTION("Parse mix")
	{
		const ful::cstr_utf8 line("\"String with spaces\" \"a_string\" true 1234 0.5");
		const auto params = engine::detail::parse_params(line);

		REQUIRE(params.size() == 5);
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[0]));
		REQUIRE(utility::holds_alternative<ful::view_utf8>(params[1]));
		REQUIRE(utility::holds_alternative<bool>(params[2]));
		REQUIRE(utility::holds_alternative<int64_t>(params[3]));
		REQUIRE(utility::holds_alternative<double>(params[4]));

		REQUIRE(utility::get<ful::view_utf8>(params[0]) == "String with spaces");
		REQUIRE(utility::get<ful::view_utf8>(params[1]) == "a_string");
		REQUIRE(utility::get<bool>(params[2]) == true);
		REQUIRE(utility::get<int64_t>(params[3]) == 1234);
		REQUIRE(utility::get<double>(params[4]) == 0.5f);
	}
}

namespace
{
	void f_empty(void *) {}
	void f_single(void *, ful::view_utf8) {}
	void f_dual1(void *, bool, int64_t) {}
	void f_dual2(void *, bool, bool) {}

	template <typename ...Parameters>
	auto create_callback(void (* fun)(void * data, Parameters...), void * data)
	{
		return engine::detail::Callback<Parameters...>(fun, data);
	}
}

// TODO: test variadic lookup
TEST_CASE("Verify variadic parsing")
{
	SECTION("Lookup no args")
	{
		auto callback = create_callback(f_empty, nullptr);
		utility::heap_vector<engine::detail::Argument> params;
		REQUIRE(callback.call(params));
		REQUIRE(params.try_emplace_back(utility::in_place_type<ful::view_utf8>));
		REQUIRE(!callback.call(params));
	}

	SECTION("Lookup args")
	{
		auto c_empty = create_callback(f_empty, nullptr);
		auto c_single = create_callback(f_single, nullptr);

		utility::heap_vector<engine::detail::Argument> params;
		REQUIRE(c_empty.call(params));		// match
		REQUIRE(!c_single.call(params));	// too few args

		REQUIRE(params.try_emplace_back(utility::in_place_type<ful::view_utf8>));

		REQUIRE(!c_empty.call(params));		// too many args
		REQUIRE(c_single.call(params));		// match
	}

	SECTION("Lookup argument missmatch")
	{
		auto c_single = create_callback(f_single, nullptr);

		utility::heap_vector<engine::detail::Argument> params;
		REQUIRE(params.try_emplace_back(utility::in_place_type<bool>, false));

		REQUIRE(!c_single.call(params));	// missmatch
	}
	SECTION("Lookup multi args match")
	{
		utility::heap_vector<engine::detail::Argument> params;

		auto call_match = create_callback(f_dual1, nullptr);
		auto call_inval = create_callback(f_dual2, nullptr);

		REQUIRE(params.try_emplace_back(utility::in_place_type<bool>, false));
		REQUIRE(!call_match.call(params));	// too few args
		REQUIRE(!call_inval.call(params));	// too few args

		REQUIRE(params.try_emplace_back(utility::in_place_type<int64_t>, 0));
		REQUIRE(call_match.call(params));	// match
		REQUIRE(!call_inval.call(params));	// missmatch
	}
	SECTION("try to send and use data")
	{
		void (*const fun)(void * data) = [](void * data){ REQUIRE(reinterpret_cast<std::intptr_t>(data) == 4711); };
		auto callback = create_callback(fun, reinterpret_cast<void *>(4711));

		utility::heap_vector<engine::detail::Argument> params;
		REQUIRE(callback.call(params));
	}
}

TEST_CASE("Console can be created and destroyed", "[engine]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::console console(nullptr);
	}
}
