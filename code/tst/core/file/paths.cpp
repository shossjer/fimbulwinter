#include "core/file/paths.hpp"

#include "ful/cstr.hpp"
#include "ful/string_compare.hpp"

#include <catch2/catch.hpp>

TEST_CASE("", "[engine][file]")
{
	CHECK(core::file::filename(ful::cstr_utf8("path/to/some.thing")) == "some");
	CHECK(core::file::filename(ful::cstr_utf8("path/to/extensionless")) == "extensionless");
	CHECK(core::file::filename(ful::cstr_utf8("filename.extension")) == "filename");
	CHECK(core::file::filename(ful::cstr_utf8("without-path-and-extension")) == "without-path-and-extension");
	CHECK(core::file::filename(ful::cstr_utf8("")) == "");
}
