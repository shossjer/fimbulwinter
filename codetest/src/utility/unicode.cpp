
#include "catch.hpp"

#include "utility/unicode.hpp"

#include <iostream>
#include <sstream>

namespace
{
	constexpr const utility::code_unit_utf8 ascii[] = "\u0024";
	constexpr const int ascii_value = 0x24;
	constexpr const int ascii_size = 1;

	constexpr const utility::code_unit_utf8 german[] = "\u00f6";
	constexpr const int german_value = 0xf6;
	constexpr const int german_size = 2;

	constexpr const utility::code_unit_utf8 snowman[] = "\u2603";
	constexpr const int snowman_value = 0x2603;
	constexpr const int snowman_size = 3;

	constexpr const utility::code_unit_utf8 smiley[] = "\U00010348";
	constexpr const int smiley_value = 0x10348;
	constexpr const int smiley_size = 4;

	constexpr const utility::code_unit_utf8 three_snowmen[] = "\u2603\u2603\u2603";
	constexpr const int three_snowmen_size = sizeof(three_snowmen) - 1;
	constexpr const int three_snowmen_length = sizeof(three_snowmen) - 1 - 3 * 2;
	//                                    two extra bytes for 3-byte characters^

	constexpr const utility::code_unit_utf8 long_text[] = "This is a story about a \u2603, named Sn\u00f6gubben, who was very good at making \u0024 since his \U00010348 was one of the best a \u2603 could have, and then he died.";
	constexpr const int long_text_size = sizeof(long_text) - 1;
	constexpr const int long_text_length = sizeof(long_text) - 1 - 1 * 1 - 2 * 2 - 1 * 3;
	//                             one extra byte for 2-byte characters^       ^       ^
	//                                    two extra bytes for 3-byte characters|       |
	//                                          three extra bytes for 4-byte characters|

	constexpr const auto long_text_symbol_1_unit = utility::UnitDifference(24);
	constexpr const auto long_text_symbol_2_unit = utility::UnitDifference(37);
	constexpr const auto long_text_symbol_3_unit = utility::UnitDifference(75);
	constexpr const auto long_text_symbol_4_unit = utility::UnitDifference(87);
	constexpr const auto long_text_symbol_5_unit = utility::UnitDifference(114);

	constexpr const auto long_text_symbol_1_point = utility::PointDifference(24);
	constexpr const auto long_text_symbol_2_point = utility::PointDifference(35);
	constexpr const auto long_text_symbol_3_point = utility::PointDifference(72);
	constexpr const auto long_text_symbol_4_point = utility::PointDifference(84);
	constexpr const auto long_text_symbol_5_point = utility::PointDifference(108);

	constexpr const utility::code_unit_utf8 short_text[] = "This is a story about a \u2603, and then he died.";
}

TEST_CASE("heap string type traits", "[utility][unicode]")
{
	using string_utf8 = utility::heap_string_utf8;

	static_assert(!std::is_trivially_default_constructible<string_utf8>::value, "");
	static_assert(std::is_default_constructible<string_utf8>::value, "");
	static_assert(!std::is_trivially_copy_constructible<string_utf8>::value, "");
	static_assert(std::is_copy_constructible<string_utf8>::value, "");
	static_assert(!std::is_trivially_move_constructible<string_utf8>::value, "");
	static_assert(std::is_move_constructible<string_utf8>::value, "");
	static_assert(!std::is_trivially_copy_assignable<string_utf8>::value, "");
	static_assert(std::is_copy_assignable<string_utf8>::value, "");
	static_assert(!std::is_trivially_move_assignable<string_utf8>::value, "");
	static_assert(std::is_move_assignable<string_utf8>::value, "");
}

TEST_CASE("static string type traits", "[utility][unicode]")
{
	using string_utf8 = utility::static_string_utf8<10>;

	static_assert(!std::is_trivially_default_constructible<string_utf8>::value, "");
	static_assert(std::is_default_constructible<string_utf8>::value, "");
	static_assert(std::is_trivially_copy_constructible<string_utf8>::value, "");
	static_assert(std::is_trivially_move_constructible<string_utf8>::value, "");
	static_assert(std::is_trivially_copy_assignable<string_utf8>::value, "");
	static_assert(std::is_trivially_move_assignable<string_utf8>::value, "");
}

TEST_CASE("static string is not bigger than necessary", "[utility][unicode]")
{
	static_assert(sizeof(utility::static_string_utf8<1>) == 2, "");
	static_assert(sizeof(utility::static_string_utf8<0xff>) == (0xff + 1), "");
	static_assert(sizeof(utility::static_string_utf8<0x100>) == (0x100 + 2), "");
	static_assert(sizeof(utility::static_string_utf8<0xffff>) == (0xffff + 1 + 2), "");
	static_assert(sizeof(utility::static_string_utf8<0x10000>) == (0x10000 + 4), "");
#ifdef _MSC_VER
	// error C2148: total size of array must not exceed 0x7fffffff bytes
#else
	static_assert(sizeof(utility::static_string_utf8<0xffffffff>) == (0xffffffffll + 1 + 4), "");
#endif
}

TEST_CASE("code point can advance correctly in utf8 strings", "[utility][unicode]")
{
	CHECK(utility::code_point::count(long_text, long_text + long_text_size) == long_text_length);

	CHECK(utility::code_point::next(long_text, 0) == 0);
	CHECK(utility::code_point::next(long_text, long_text_symbol_1_point.get()) == long_text_symbol_1_unit.get());
	CHECK(utility::code_point::next(long_text, long_text_symbol_2_point.get()) == long_text_symbol_2_unit.get());
	CHECK(utility::code_point::next(long_text, long_text_symbol_3_point.get()) == long_text_symbol_3_unit.get());
	CHECK(utility::code_point::next(long_text, long_text_symbol_4_point.get()) == long_text_symbol_4_unit.get());
	CHECK(utility::code_point::next(long_text, long_text_symbol_5_point.get()) == long_text_symbol_5_unit.get());

	CHECK(utility::code_point::previous(long_text + long_text_size, 0) == 0);
	CHECK(utility::code_point::previous(long_text + long_text_size, long_text_length - long_text_symbol_5_point.get()) == long_text_size - long_text_symbol_5_unit.get());
	CHECK(utility::code_point::previous(long_text + long_text_size, long_text_length - long_text_symbol_4_point.get()) == long_text_size - long_text_symbol_4_unit.get());
	CHECK(utility::code_point::previous(long_text + long_text_size, long_text_length - long_text_symbol_3_point.get()) == long_text_size - long_text_symbol_3_unit.get());
	CHECK(utility::code_point::previous(long_text + long_text_size, long_text_length - long_text_symbol_2_point.get()) == long_text_size - long_text_symbol_2_unit.get());
	CHECK(utility::code_point::previous(long_text + long_text_size, long_text_length - long_text_symbol_1_point.get()) == long_text_size - long_text_symbol_1_unit.get());
}

TEST_CASE("string views", "[utility][unicode]")
{
	utility::string_view_utf8 v = long_text;
	CHECK(v.size() == long_text_size);
	CHECK(v.length() == long_text_length);
	CHECK(!v.empty());

	SECTION("can be iterated over")
	{
		int length = 0;
		for (auto it = v.begin(); it != v.end(); ++it)
			length++;

		CHECK(length == long_text_length);
	}

	SECTION("can be indexed")
	{
		CHECK(v[long_text_symbol_1_unit] == utility::code_point(snowman));
		CHECK(v[long_text_symbol_2_unit] == utility::code_point(german));
		CHECK(v[long_text_symbol_3_unit] == utility::code_point(ascii));
		CHECK(v[long_text_symbol_4_unit] == utility::code_point(smiley));
		CHECK(v[long_text_symbol_5_unit] == utility::code_point(snowman));

		CHECK(v[long_text_symbol_1_point] == utility::code_point(snowman));
		CHECK(v[long_text_symbol_2_point] == utility::code_point(german));
		CHECK(v[long_text_symbol_3_point] == utility::code_point(ascii));
		CHECK(v[long_text_symbol_4_point] == utility::code_point(smiley));
		CHECK(v[long_text_symbol_5_point] == utility::code_point(snowman));
	}

	SECTION("can be trimmed")
	{
		v.remove_prefix(long_text_symbol_3_point - 0);
		CHECK(v.front() == utility::code_point(ascii));
		CHECK(v.starts_with(utility::code_point(ascii)));

		v.remove_suffix(long_text_length - (long_text_symbol_4_point + 1));
		CHECK(v.back() == utility::code_point(smiley));
		CHECK(v.ends_with(utility::code_point(smiley)));
	}

	SECTION("can be compared")
	{
		SECTION("to itself")
		{
			CHECK(v == v);
			CHECK_FALSE(v != v);
			CHECK_FALSE(v < v);
			CHECK(v <= v);
			CHECK_FALSE(v > v);
			CHECK(v >= v);
		}

		SECTION("to nullptr")
		{
			CHECK_FALSE(v == nullptr);
			CHECK(v != nullptr);
			CHECK_FALSE(v < nullptr);
			CHECK_FALSE(v <= nullptr);
			CHECK(v > nullptr);
			CHECK(v >= nullptr);

			CHECK_FALSE(nullptr == v);
			CHECK(nullptr != v);
			CHECK(nullptr < v);
			CHECK(nullptr <= v);
			CHECK_FALSE(nullptr > v);
			CHECK_FALSE(nullptr >= v);

			const utility::code_unit_utf8 * const null = nullptr;

			CHECK_FALSE(v == null);
			CHECK(v != null);
			CHECK_FALSE(v < null);
			CHECK_FALSE(v <= null);
			CHECK(v > null);
			CHECK(v >= null);

			CHECK_FALSE(null == v);
			CHECK(null != v);
			CHECK(null < v);
			CHECK(null <= v);
			CHECK_FALSE(null > v);
			CHECK_FALSE(null >= v);
		}

		SECTION("to empty string views")
		{
			const utility::string_view_utf8 w = "";

			CHECK_FALSE(v == w);
			CHECK(v != w);
			CHECK_FALSE(v < w);
			CHECK_FALSE(v <= w);
			CHECK(v > w);
			CHECK(v >= w);

			CHECK_FALSE(w == v);
			CHECK(w != v);
			CHECK(w < v);
			CHECK(w <= v);
			CHECK_FALSE(w > v);
			CHECK_FALSE(w >= v);
		}

		SECTION("to other string views")
		{
			const utility::string_view_utf8 w = short_text;

			CHECK_FALSE(v == w);
			CHECK(v != w);
			CHECK_FALSE(v < w);
			CHECK_FALSE(v <= w);
			CHECK(v > w);
			CHECK(v >= w);

			CHECK_FALSE(w == v);
			CHECK(w != v);
			CHECK(w < v);
			CHECK(w <= v);
			CHECK_FALSE(w > v);
			CHECK_FALSE(w >= v);
		}

		SECTION("to null terminated strings")
		{
			CHECK(v == long_text);
			CHECK_FALSE(v != long_text);
			CHECK_FALSE(v < long_text);
			CHECK(v <= long_text);
			CHECK_FALSE(v > long_text);
			CHECK(v >= long_text);

			CHECK(long_text == v);
			CHECK_FALSE(long_text != v);
			CHECK_FALSE(long_text < v);
			CHECK(long_text <= v);
			CHECK_FALSE(long_text > v);
			CHECK(long_text >= v);

			CHECK_FALSE(v == short_text);
			CHECK(v != short_text);
			CHECK_FALSE(v < short_text);
			CHECK_FALSE(v <= short_text);
			CHECK(v > short_text);
			CHECK(v >= short_text);

			CHECK_FALSE(short_text == v);
			CHECK(short_text != v);
			CHECK(short_text < v);
			CHECK(short_text <= v);
			CHECK_FALSE(short_text > v);
			CHECK_FALSE(short_text >= v);
		}
	}

	SECTION("can be ostreamed")
	{
		std::ostringstream os;
		os << v;
		CHECK(std::strncmp(os.str().c_str(), long_text, long_text_size) == 0);
	}
}

TEST_CASE("three snowmen is a crowd", "[utility][unicode]")
{
	const auto snowman_cp = utility::code_point(snowman);

	// CHECK_THROWS(utility::static_string_utf8<9>(3, snowman_cp));
	const auto s = utility::static_string_utf8<10>(3, snowman_cp);
	CHECK(s.size() == 3 * snowman_size);
	CHECK(s.length() == 3);

	for (auto cp : s)
	{
		CHECK(cp == snowman_cp);
	}

	CHECK(std::strcmp(s.data(), three_snowmen) == 0);
}

TEST_CASE("store ascii in code point", "[utility][unicode]")
{
	const auto ascii_cp = utility::code_point(ascii);
	CHECK(ascii_cp.size() == ascii_size);
	CHECK(ascii_cp.get() == ascii_value);

	SECTION("extract code units from code point")
	{
		utility::code_unit_utf8 chars[4];
		ascii_cp.get(chars);
		CHECK(std::strncmp(chars, ascii, ascii_cp.size()) == 0);
	}

	SECTION("ostream code point")
	{
		std::ostringstream os;
		os << ascii_cp;
		CHECK(std::strncmp(os.str().c_str(), ascii, ascii_cp.size()) == 0);
	}

	SECTION("copy constructed code point should be the same")
	{
		const auto cpy = ascii_cp;
		CHECK(cpy.size() == ascii_size);
		CHECK(cpy.get() == ascii_value);

		CHECK(cpy == ascii_cp);
		CHECK_FALSE(cpy != ascii_cp);
		CHECK_FALSE(cpy < ascii_cp);
		CHECK(cpy <= ascii_cp);
		CHECK_FALSE(cpy > ascii_cp);
		CHECK(cpy >= ascii_cp);
	}

	SECTION("copy assigned code point should be the same")
	{
		utility::code_point cpy;
		cpy = ascii_cp;
		CHECK(cpy.size() == ascii_size);
		CHECK(cpy.get() == ascii_value);

		CHECK(cpy == ascii_cp);
		CHECK_FALSE(cpy != ascii_cp);
		CHECK_FALSE(cpy < ascii_cp);
		CHECK(cpy <= ascii_cp);
		CHECK_FALSE(cpy > ascii_cp);
		CHECK(cpy >= ascii_cp);
	}
}

TEST_CASE("store german in code point", "[utility][unicode]")
{
	const auto german_cp = utility::code_point(german);
	CHECK(german_cp.size() == german_size);
	CHECK(german_cp.get() == german_value);

	SECTION("extract code units from code point")
	{
		utility::code_unit_utf8 chars[4];
		german_cp.get(chars);
		CHECK(std::strncmp(chars, german, german_cp.size()) == 0);
	}

	SECTION("ostream code point")
	{
		std::ostringstream os;
		os << german_cp;
		CHECK(std::strncmp(os.str().c_str(), german, german_cp.size()) == 0);
	}

	SECTION("copy constructed code point should be the same")
	{
		const auto cpy = german_cp;
		CHECK(cpy.size() == german_size);
		CHECK(cpy.get() == german_value);

		CHECK(cpy == german_cp);
		CHECK_FALSE(cpy != german_cp);
		CHECK_FALSE(cpy < german_cp);
		CHECK(cpy <= german_cp);
		CHECK_FALSE(cpy > german_cp);
		CHECK(cpy >= german_cp);
	}

	SECTION("copy assigned code point should be the same")
	{
		utility::code_point cpy;
		cpy = german_cp;
		CHECK(cpy.size() == german_size);
		CHECK(cpy.get() == german_value);

		CHECK(cpy == german_cp);
		CHECK_FALSE(cpy != german_cp);
		CHECK_FALSE(cpy < german_cp);
		CHECK(cpy <= german_cp);
		CHECK_FALSE(cpy > german_cp);
		CHECK(cpy >= german_cp);
	}
}

TEST_CASE("store snowman in code point", "[utility][unicode]")
{
	const auto snowman_cp = utility::code_point(snowman);
	CHECK(snowman_cp.size() == snowman_size);
	CHECK(snowman_cp.get() == snowman_value);

	SECTION("extract code units from code point")
	{
		utility::code_unit_utf8 chars[4];
		snowman_cp.get(chars);
		CHECK(std::strncmp(chars, snowman, snowman_cp.size()) == 0);
	}

	SECTION("ostream code point")
	{
		std::ostringstream os;
		os << snowman_cp;
		CHECK(std::strncmp(os.str().c_str(), snowman, snowman_cp.size()) == 0);
	}

	SECTION("copy constructed code point should be the same")
	{
		const auto cpy = snowman_cp;
		CHECK(cpy.size() == snowman_size);
		CHECK(cpy.get() == snowman_value);

		CHECK(cpy == snowman_cp);
		CHECK_FALSE(cpy != snowman_cp);
		CHECK_FALSE(cpy < snowman_cp);
		CHECK(cpy <= snowman_cp);
		CHECK_FALSE(cpy > snowman_cp);
		CHECK(cpy >= snowman_cp);
	}

	SECTION("copy assigned code point should be the same")
	{
		utility::code_point cpy;
		cpy = snowman_cp;
		CHECK(cpy.size() == snowman_size);
		CHECK(cpy.get() == snowman_value);

		CHECK(cpy == snowman_cp);
		CHECK_FALSE(cpy != snowman_cp);
		CHECK_FALSE(cpy < snowman_cp);
		CHECK(cpy <= snowman_cp);
		CHECK_FALSE(cpy > snowman_cp);
		CHECK(cpy >= snowman_cp);
	}
}

TEST_CASE("store smiley in code point", "[utility][unicode]")
{
	const auto smiley_cp = utility::code_point(smiley);
	CHECK(smiley_cp.size() == smiley_size);
	CHECK(smiley_cp.get() == smiley_value);

	SECTION("extract code units from code point")
	{
		utility::code_unit_utf8 chars[4];
		smiley_cp.get(chars);
		CHECK(std::strncmp(chars, smiley, smiley_cp.size()) == 0);
	}

	SECTION("ostream code point")
	{
		std::ostringstream os;
		os << smiley_cp;
		CHECK(std::strncmp(os.str().c_str(), smiley, smiley_cp.size()) == 0);
	}

	SECTION("copy constructed code point should be the same")
	{
		const auto cpy = smiley_cp;
		CHECK(cpy.size() == smiley_size);
		CHECK(cpy.get() == smiley_value);

		CHECK(cpy == smiley_cp);
		CHECK_FALSE(cpy != smiley_cp);
		CHECK_FALSE(cpy < smiley_cp);
		CHECK(cpy <= smiley_cp);
		CHECK_FALSE(cpy > smiley_cp);
		CHECK(cpy >= smiley_cp);
	}

	SECTION("copy assigned code point should be the same")
	{
		utility::code_point cpy;
		cpy = smiley_cp;
		CHECK(cpy.size() == smiley_size);
		CHECK(cpy.get() == smiley_value);

		CHECK(cpy == smiley_cp);
		CHECK_FALSE(cpy != smiley_cp);
		CHECK_FALSE(cpy < smiley_cp);
		CHECK(cpy <= smiley_cp);
		CHECK_FALSE(cpy > smiley_cp);
		CHECK(cpy >= smiley_cp);
	}
}

TEST_CASE("code point can hold the null character", "[utility][unicode]")
{
	const auto empty = utility::code_point("");
	CHECK(empty.size() == 1);
	CHECK(empty.get() == '\0');

	SECTION("empty and null are the same")
	{
		const auto null = utility::code_point(nullptr);
		CHECK(null.size() == 1);
		CHECK(null.get() == '\0');

		CHECK(empty == null);
		CHECK_FALSE(empty != null);
		CHECK_FALSE(empty < null);
		CHECK(empty <= null);
		CHECK_FALSE(empty > null);
		CHECK(empty >= null);
	}

	SECTION("null pointers that are not std::nullptr_t should be fine aswell")
	{
		utility::code_unit_utf8* const ptr = nullptr;
		const auto null = utility::code_point(ptr);

		CHECK(null.size() == 1);
		CHECK(null.get() == '\0');

		CHECK(empty == null);
		CHECK_FALSE(empty != null);
		CHECK_FALSE(empty < null);
		CHECK(empty <= null);
		CHECK_FALSE(empty > null);
		CHECK(empty >= null);
	}

	SECTION("empty is different to snowmen")
	{
		const auto snowman_cp = utility::code_point(snowman);
		CHECK(snowman_cp.size() == snowman_size);
		CHECK(snowman_cp.get() == snowman_value);

		CHECK_FALSE(empty == snowman_cp);
		CHECK(empty != snowman_cp);
		CHECK(empty < snowman_cp);
		CHECK(empty <= snowman_cp);
		CHECK_FALSE(empty > snowman_cp);
		CHECK_FALSE(empty >= snowman_cp);

		CHECK_FALSE(snowman_cp == empty);
		CHECK(snowman_cp != empty);
		CHECK_FALSE(snowman_cp < empty);
		CHECK_FALSE(snowman_cp <= empty);
		CHECK(snowman_cp > empty);
		CHECK(snowman_cp >= empty);
	}
}

TEST_CASE("static string", "[utility][unicode]")
{
	using string_utf8 = utility::static_string_utf8<(long_text_size + 1)>;

	const string_utf8 s = long_text;
	CHECK_FALSE(s.empty());
	CHECK(s.size() == long_text_size);
	CHECK(s.length() == long_text_length);

	const utility::string_view_utf8 sv = s;
	CHECK_FALSE(sv.empty());
	CHECK(sv.size() == long_text_size);
	CHECK(sv.length() == long_text_length);
	CHECK(sv.compare(long_text) == 0);

	CHECK(sv[long_text_symbol_1_unit] == utility::code_point(snowman));
	CHECK(sv[long_text_symbol_2_unit] == utility::code_point(german));
	CHECK(sv[long_text_symbol_3_unit] == utility::code_point(ascii));
	CHECK(sv[long_text_symbol_4_unit] == utility::code_point(smiley));
	CHECK(sv[long_text_symbol_5_unit] == utility::code_point(snowman));

	CHECK(sv[long_text_symbol_1_point] == utility::code_point(snowman));
	CHECK(sv[long_text_symbol_2_point] == utility::code_point(german));
	CHECK(sv[long_text_symbol_3_point] == utility::code_point(ascii));
	CHECK(sv[long_text_symbol_4_point] == utility::code_point(smiley));
	CHECK(sv[long_text_symbol_5_point] == utility::code_point(snowman));
}

TEST_CASE("static strings can be memcpy-ed", "[utility][unicode][storage]")
{
	const utility::static_string_utf8<(three_snowmen_size + 1)> three_snowmen_strings[3] = { three_snowmen, three_snowmen, three_snowmen };

	using storage_type = utility::static_storage<utility::static_string_utf8<(three_snowmen_size + 1)>, 3>;
	storage_type storage;

	static_assert(utility::storage_traits<storage_type>::trivial_allocate::value, "");
	// storage.allocate(3);

	static_assert(storage_type::storing_trivially_copyable::value, "");
	storage.memcpy_range(0, three_snowmen_strings, three_snowmen_strings + 3);

	for (int i = 0; i < 3; i++)
	{
		CHECK(storage.data()[i].compare(three_snowmen) == 0);
	}

	static_assert(utility::storage_traits<storage_type>::trivial_allocate::value, "");
	static_assert(storage_type::storing_trivially_copyable::value, "");
	storage_type copy = storage;

	for (int i = 0; i < 3; i++)
	{
		CHECK(storage.data()[i].compare(three_snowmen) == 0);
		CHECK(copy.data()[i].compare(three_snowmen) == 0);
	}

	static_assert(storage_type::storing_trivially_destructible::value, "");
	// storage.destruct_range(0, 3);

	static_assert(utility::storage_traits<storage_type>::trivial_deallocate::value, "");
	// storage.deallocate(3);

	static_assert(storage_type::storing_trivially_copyable::value, "");
	storage = std::move(copy);

	for (int i = 0; i < 3; i++)
	{
		CHECK(storage.data()[i].compare(three_snowmen) == 0);
	}

	static_assert(storage_type::storing_trivially_destructible::value, "");
	// storage.destruct_range(0, 3);

	static_assert(utility::storage_traits<storage_type>::trivial_deallocate::value, "");
	// storage.deallocate(3);
}

TEST_CASE("heap string", "[utility][unicode]")
{
	using string_utf8 = utility::heap_string_utf8;

	utility::heap_string_utf8 s = long_text;
	CHECK_FALSE(s.empty());
	CHECK(s.size() == long_text_size);
	CHECK(s.length() == long_text_length);

	SECTION("copy construct")
	{
		utility::heap_string_utf8 t = s;
		CHECK_FALSE(t.empty());
		CHECK(t.size() == long_text_size);
		CHECK(t.length() == long_text_length);
	}

	SECTION("copy assign")
	{
		utility::heap_string_utf8 t;
		t = s;
		CHECK_FALSE(t.empty());
		CHECK(t.size() == long_text_size);
		CHECK(t.length() == long_text_length);
	}

	SECTION("move construct")
	{
		utility::heap_string_utf8 t = std::move(s);
		CHECK_FALSE(t.empty());
		CHECK(t.size() == long_text_size);
		CHECK(t.length() == long_text_length);

		CHECK(s.empty());
		CHECK(s.size() == 0);
		CHECK(s.length() == 0);
	}

	SECTION("move assign")
	{
		utility::heap_string_utf8 t;
		t = std::move(s);
		CHECK_FALSE(t.empty());
		CHECK(t.size() == long_text_size);
		CHECK(t.length() == long_text_length);

		CHECK(s.empty());
		CHECK(s.size() == 0);
		CHECK(s.length() == 0);
	}

	SECTION("")
	{
		const utility::string_view_utf8 sv = s;
		CHECK_FALSE(sv.empty());
		CHECK(sv.size() == long_text_size);
		CHECK(sv.length() == long_text_length);
		CHECK(sv.compare(long_text) == 0);

		CHECK(sv[long_text_symbol_1_unit] == utility::code_point(snowman));
		CHECK(sv[long_text_symbol_2_unit] == utility::code_point(german));
		CHECK(sv[long_text_symbol_3_unit] == utility::code_point(ascii));
		CHECK(sv[long_text_symbol_4_unit] == utility::code_point(smiley));
		CHECK(sv[long_text_symbol_5_unit] == utility::code_point(snowman));

		CHECK(sv[long_text_symbol_1_point] == utility::code_point(snowman));
		CHECK(sv[long_text_symbol_2_point] == utility::code_point(german));
		CHECK(sv[long_text_symbol_3_point] == utility::code_point(ascii));
		CHECK(sv[long_text_symbol_4_point] == utility::code_point(smiley));
		CHECK(sv[long_text_symbol_5_point] == utility::code_point(snowman));
	}
}

TEST_CASE("ostream string types", "[utility][unicode]")
{
	const utility::static_string_utf8<4> s = snowman;
	{
		std::ostringstream os;
		os << s;
		CHECK(std::strcmp(os.str().c_str(), snowman) == 0);
	}
	const utility::string_view_utf8 sv = s;
	{
		std::ostringstream os;
		os << sv;
		CHECK(std::strcmp(os.str().c_str(), snowman) == 0);
	}
}
