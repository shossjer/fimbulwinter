#include "utility/regex.hpp"

#include <catch2/catch.hpp>

TEST_CASE("regex matching strings", "[utility][regex]")
{
	const char * const text = "abcdef?";

	const auto alphabet = rex::match(text, text + 6);

	SECTION("can match the empty string")
	{
		const auto empty = alphabet >> rex::string("");
		REQUIRE(empty.first);
		CHECK(empty.second == text + 0);
	}

	SECTION("can match partial results, repeatedly")
	{
		const auto after_abc = alphabet >> rex::string("abc");
		REQUIRE(after_abc.first);
		REQUIRE(after_abc.second == text + 3);

		const auto after_de = after_abc >> rex::string("de");
		REQUIRE(after_de.first);
		REQUIRE(after_de.second == text + 5);

		const auto after_f = after_de >> rex::string("f");
		REQUIRE(after_f.first);
		REQUIRE(after_f.second == text + 6);

		SECTION("but not past the end")
		{
			const auto fail = after_f >> rex::string("?");
			REQUIRE_FALSE(fail.first);
			CHECK(fail.second == text + 6);
		}
	}

	SECTION("can match partial results, as one expression")
	{
		const auto after_abcdef = alphabet >> rex::string("abc") >> rex::string("def");
		REQUIRE(after_abcdef.first);
		REQUIRE(after_abcdef.second == text + 6);

		SECTION("but not past the end")
		{
			const auto fail = after_abcdef >> rex::string("?");
			REQUIRE_FALSE(fail.first);
			CHECK(fail.second == text + 6);
		}
	}

	SECTION("can match everything")
	{
		const auto after_abcdef = alphabet >> rex::string("abcdef");
		REQUIRE(after_abcdef.first);
		REQUIRE(after_abcdef.second == text + 6);

		SECTION("but not past the end")
		{
			const auto fail = after_abcdef >> rex::string("?");
			REQUIRE_FALSE(fail.first);
			CHECK(fail.second == text + 6);
		}
	}

	SECTION("cannot match anything wrong")
	{
		const auto fail = alphabet >> rex::string("fedcba");
		REQUIRE_FALSE(fail.first);
		CHECK(fail.second == text + 0);
	}

	SECTION("reports partial results on unsuccessful matches")
	{
		const auto fail = alphabet >> rex::string("abxxef");
		REQUIRE_FALSE(fail.first);
		CHECK(fail.second == text + 2);
	}

	SECTION("reports partial results on excess matches")
	{
		const auto fail = alphabet >> rex::string("abcdef?");
		REQUIRE_FALSE(fail.first);
		CHECK(fail.second == text + 6);
	}

	SECTION("propagates errors when chaining")
	{
		const auto abxc = alphabet >> rex::string("abx") >> rex::string("c");
		REQUIRE_FALSE(abxc.first);
		CHECK(abxc.second == text + 2);
	}
}

TEST_CASE("", "[utility][regex]")
{
	const char * const text = "aaabcbcbd";

	const auto letters = rex::match(text, text + 9);

	const auto after_aaa = letters >> +rex::string("a");
	CHECK(after_aaa.first);
	CHECK(after_aaa.second == text + 3);

	const auto after_bcbc = after_aaa >> +rex::string("bc");
	CHECK(after_bcbc.first);
	CHECK(after_bcbc.second == text + 7);

	SECTION("")
	{
		const auto fail = after_bcbc >> +rex::string("bb");
		CHECK_FALSE(fail.first);
		CHECK(fail.second == text + 8);
	}
}

TEST_CASE("", "[utility][regex]")
{
	const char * const text = "aaabcbcbd";

	const auto letters = rex::match(text, text + 9);

	const auto after_all_q = letters >> *rex::string("q");
	CHECK(after_all_q.first);
	CHECK(after_all_q.second == text + 0);

	const auto after_all_a = after_all_q >> *rex::string("a");
	CHECK(after_all_a.first);
	CHECK(after_all_a.second == text + 3);

	const auto after_all_bc = after_all_a >> *rex::string("bc");
	CHECK(after_all_bc.first);
	CHECK(after_all_bc.second == text + 7);

	const auto after_all_bb = after_all_bc >> *rex::string("bb");
	CHECK(after_all_bb.first);
	CHECK(after_all_bb.second == text + 7);
}

TEST_CASE("regex matching newlines", "[utility][regex]")
{
	const char * const text = "\n\r\r\n";

	const auto newlines = rex::match(text, text + 13);

	const auto lin_newline = newlines >> rex::newline;
	CHECK(lin_newline.first);
	CHECK(lin_newline.second == text + 1);

	const auto mac_newline = lin_newline >> rex::newline;
	CHECK(mac_newline.first);
	CHECK(mac_newline.second == text + 2);

	const auto win_newline = mac_newline >> rex::newline;
	CHECK(win_newline.first);
	CHECK(win_newline.second == text + 4);
}
