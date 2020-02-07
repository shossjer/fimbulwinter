#include "utility/crypto/crc.hpp"

#include <catch/catch.hpp>

TEST_CASE( "crc32", "[utility][crypto]" )
{
	constexpr uint32_t code1 = utility::crypto::crc32("1");
	static_assert(code1 == 0x83dcefb7, "");

	constexpr uint32_t code2 = utility::crypto::crc32("11", 1);
	static_assert(code2 == 0x83dcefb7, "");

	const auto str3 = std::string("1");
	uint32_t code3 = utility::crypto::crc32(str3.data(), str3.length());
	REQUIRE(code1 == code3);

	switch (code3)
	{
	case code1:
		REQUIRE(true);
		break;
	default:
		REQUIRE(false);
		break;
	}
}
