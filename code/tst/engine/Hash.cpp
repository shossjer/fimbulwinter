#include "engine/Hash.hpp"
#include "engine/HashTable.hpp"

#include <catch2/catch.hpp>

#include <sstream>

static_hashes("rEgIsTeReD StRiNg");

TEST_CASE("A zero initialized Hash", "[engine][hash]")
{
	engine::Hash hash{};

	SECTION("contains the underlying value zero")
	{
		CHECK(hash.value() == 0);
	}

	SECTION("is the same as Hash of the empty string")
	{
		CHECK(hash == engine::Hash(""));
	}
}

#if MODE_DEBUG

TEST_CASE("Ostreaming a Hash", "[engine][hash]")
{
	std::ostringstream ostream;

	SECTION("that is zero initialized, gives zero back")
	{
		ostream << engine::Hash{};

		CHECK(ostream.str() == "0(\"\")");
	}

	SECTION("with a registered string, gives said string back")
	{
		ostream << engine::Hash("rEgIsTeReD StRiNg");

		CHECK(ostream.str() == "3595138907(\"rEgIsTeReD StRiNg\")");
	}

	SECTION("with a unregistered string, gives no string back")
	{
		ostream << engine::Hash("unregistered string");

		CHECK(ostream.str() == "995593901( ? )");
	}
}

#endif
