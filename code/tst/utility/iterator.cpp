#include "utility/iterator.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <vector>

namespace
{
	struct test_range
	{
		char * begin();
		const char * begin() const;
		const char * cbegin() const;
		char * end();
		const char * end() const;
		const char * cend() const;

		std::reverse_iterator<char *> rbegin();
		std::reverse_iterator<const char *> rbegin() const;
		std::reverse_iterator<const char *> crbegin() const;
		std::reverse_iterator<char *> rend();
		std::reverse_iterator<const char *> rend() const;
		std::reverse_iterator<const char *> crend() const;
	};
}

TEST_CASE("iterator properties", "[utility][iterator]")
{
	using ext::begin;
	using ext::rbegin;
	using ext::cbegin;
	using ext::crbegin;

	using ext::end;
	using ext::rend;
	using ext::cend;
	using ext::crend;

	SECTION("on c-array")
	{
		int a[3];
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::array")
	{
		std::array<int, 5> a;
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::initializer_list")
	{
		// todo moving initializer_list will not result in a move_iterator due to the way its begin/end function is implemented, so for now it is not supported

		std::initializer_list<int> a;
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		//CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::list")
	{
		std::list<int> a;
		CHECK(!ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::map")
	{
		std::map<int, float> a;
		CHECK(!ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::set")
	{
		std::set<int> a;
		CHECK(!ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on std::unordered_map")
	{
		std::unordered_map<int, float> a;
		CHECK(!ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);
	}

	SECTION("on std::unordered_set")
	{
		std::unordered_set<int> a;
		CHECK(!ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);
	}

	SECTION("on std::valarray")
	{
		std::valarray<int> a;
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);
	}

	SECTION("on std::vector")
	{
		std::vector<int> a;
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}

	SECTION("on test_range")
	{
		test_range a;
		CHECK(ext::is_contiguous_iterator<decltype(begin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(end(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(end(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(end(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(begin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(end(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(rend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(rend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(cbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(cend(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(a))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(cend(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cbegin(std::move(a)))>::value);
		CHECK(!ext::is_reverse_iterator<decltype(cend(std::move(a)))>::value);

		CHECK(ext::is_contiguous_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crbegin(a))>::value);
		CHECK(!ext::is_move_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(a))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(a))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_contiguous_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_move_iterator<decltype(crend(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crbegin(std::move(a)))>::value);
		CHECK(ext::is_reverse_iterator<decltype(crend(std::move(a)))>::value);
	}
}

TEST_CASE("reverse works on", "[utility]")
{
	SECTION("arrays")
	{
		const int values[] = {0, 1, 2, 3, 4};

		int count = 0;
		for (auto x : utility::reverse(values))
		{
			CHECK(x == values[4 - count]);
			count++;
		}
		REQUIRE(count == 5);
	}

	SECTION("vectors")
	{
		const std::vector<float> values = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f};

		int count = 0;
		for (auto x : utility::reverse(values))
		{
			CHECK(static_cast<float>(x) == values[6 - count]);
			count++;
		}
		REQUIRE(count == 7);
	}
}
