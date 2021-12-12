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
		CHECK(static_cast<engine::Hash::value_type>(hash) == 0);
	}

	SECTION("is the same as Hash of the empty string")
	{
		CHECK(hash == engine::Hash(""));
	}
}
