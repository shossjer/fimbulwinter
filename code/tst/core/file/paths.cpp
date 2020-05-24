#include "core/file/paths.hpp"

#include "utility/unicode.hpp"

#include <catch2/catch.hpp>

TEST_CASE("", "[engine][file]")
{
	CHECK(core::file::filename(utility::string_view_utf8(u8"path/to/some.thing")) == u8"some");
	CHECK(core::file::filename(utility::string_view_utf8(u8"path/to/extensionless")) == u8"extensionless");
	CHECK(core::file::filename(utility::string_view_utf8(u8"filename.extension")) == u8"filename");
	CHECK(core::file::filename(utility::string_view_utf8(u8"without-path-and-extension")) == u8"without-path-and-extension");
	CHECK(core::file::filename(utility::string_view_utf8(u8"")) == u8"");
}
