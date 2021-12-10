#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/HashTable.hpp"
#include "engine/Token.hpp"

#include <catch2/catch.hpp>

#undef small

static_hashes("eman");

TEST_CASE("A zero initialized Token", "[engine][token]")
{
	engine::Token token{};

	SECTION("contains the underlying value zero")
	{
		CHECK(token.value() == 0);
#if MODE_DEBUG
		CHECK(token.type() == 0);
#endif
	}

	SECTION("is the same an Token created with zeros")
	{
		CHECK(token.value() == engine::Token(0, 0).value());
#if MODE_DEBUG
		CHECK(token.type() == engine::Token(0, 0).type());
#endif
	}
}

TEST_CASE("Token has normal value semantics", "[engine][token]")
{
	engine::Token a(17, 19);

	SECTION("w.r.t copying")
	{
		engine::Token b = a;

		CHECK(b.value() == 17);
		CHECK(a.value() == 17);
#if MODE_DEBUG
		CHECK(b.type() == 19);
		CHECK(a.type() == 19);
#endif

		b = a;

		CHECK(b.value() == 17);
		CHECK(a.value() == 17);
#if MODE_DEBUG
		CHECK(b.type() == 19);
		CHECK(a.type() == 19);
#endif
	}

	SECTION("w.r.t moving")
	{
		engine::Token b = std::move(a);

		CHECK(b.value() == 17);
		CHECK(a.value() == 17);
#if MODE_DEBUG
		CHECK(b.type() == 19);
		CHECK(a.type() == 19);
#endif

		b = std::move(a);

		CHECK(b.value() == 17);
		CHECK(a.value() == 17);
#if MODE_DEBUG
		CHECK(b.type() == 19);
		CHECK(a.type() == 19);
#endif
	}
}

TEST_CASE("Token can be constructed", "[engine][assert][entity][token]")
{
	SECTION("from Asset")
	{
		engine::Token token(engine::Asset("name"));

		CHECK(token.value() == 1579384326);
#if MODE_DEBUG
		CHECK(token.type() == 700617891);
#endif
	}

	SECTION("from Entity")
	{
		engine::Token token(engine::Entity(11));

		CHECK(token.value() == 11);
#if MODE_DEBUG
		CHECK(token.type() == 587318827);
#endif
	}

	SECTION("from Hash")
	{
		engine::Token token(engine::Hash("eman"));

		CHECK(token.value() == 3143603943);
#if MODE_DEBUG
		CHECK(token.type() == 137105154);
#endif
	}

	SECTION("from int")
	{
		engine::Token token(7);

		CHECK(token.value() == 7);
#if MODE_DEBUG
		CHECK(token.type() == 340908721);
#endif
	}
}

TEST_CASE("Token compares", "[engine][token]")
{
	engine::Token small(char(2));
	engine::Token big(int(17));
	engine::Token also_small(int(2));

	SECTION("w.r.t. ==")
	{
		CHECK(small == small);
		CHECK(big == big);

		CHECK_FALSE(small == big);

		CHECK(small == also_small);
		CHECK_FALSE(big == also_small);
	}

	SECTION("w.r.t. !=")
	{
		CHECK_FALSE(small != small);
		CHECK_FALSE(big != big);

		CHECK(small != big);

		CHECK_FALSE(small != also_small);
		CHECK(big != also_small);
	}

	SECTION("w.r.t. <")
	{
		CHECK_FALSE(small < small);
		CHECK_FALSE(big < big);

		CHECK(small < big);

		CHECK_FALSE(small < also_small);
		CHECK_FALSE(big < also_small);
	}

	SECTION("w.r.t. <=")
	{
		CHECK(small <= small);
		CHECK(big <= big);

		CHECK(small <= big);

		CHECK(small <= also_small);
		CHECK_FALSE(big <= also_small);
	}

	SECTION("w.r.t. >")
	{
		CHECK_FALSE(small > small);
		CHECK_FALSE(big > big);

		CHECK_FALSE(small > big);

		CHECK_FALSE(small > also_small);
		CHECK(big > also_small);
	}

	SECTION("w.r.t. >=")
	{
		CHECK(small >= small);
		CHECK(big >= big);

		CHECK_FALSE(small >= big);

		CHECK(small >= also_small);
		CHECK(big >= also_small);
	}
}
