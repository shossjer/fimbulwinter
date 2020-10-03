#include "utility/regex.hpp"

#include <catch2/catch.hpp>

namespace
{
	template <char C>
	struct character_class
	{
		explicit character_class() = default;

		template <typename BeginIt, typename EndIt, bool Negation>
		rex::match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
		{
			if (begin == end)
				return {false, begin};

			const bool is_character = *begin == C;
			if (is_character != Negation)
			{
				++begin;
			}

			return {is_character != Negation, begin};
		}
	};

	const auto class_a = rex::pattern<character_class<'a'>>();
	const auto class_b = rex::pattern<character_class<'b'>>();
	const auto class_c = rex::pattern<character_class<'c'>>();
}

TEST_CASE("", "[utility][regex]")
{
	const char * const text = "aab";

	auto parser = rex::parse(text + 0, text + 3);

	SECTION("")
	{
		const auto result = parser.match(class_a >> class_a >> !class_a);
		CHECK(result.first);
		CHECK(result.second == text + 3);
	}

	SECTION("")
	{
		const auto result = parser.match(*class_a >> class_a >> !class_a);
		CHECK_FALSE(result.first);
		CHECK(result.second == text + 2);
	}

	SECTION("")
	{
		const auto result = parser.match(+class_a >> class_a >> !class_a);
		CHECK_FALSE(result.first);
		CHECK(result.second == text + 2);
	}

	SECTION("")
	{
		const auto result = parser.match(-class_a >> class_a >> !class_a);
		CHECK(result.first);
		CHECK(result.second == text + 3);
	}

	SECTION("")
	{
		const auto result = parser.match((class_a | (class_a >> class_a)) >> !class_a);
		CHECK_FALSE(result.first);
		CHECK(result.second == text + 1);
	}

	SECTION("")
	{
		const auto result = parser.match(((class_a >> class_a) | class_a) >> !class_a);
		CHECK(result.first);
		CHECK(result.second == text + 3);
	}
}

TEST_CASE("", "[utility][regex]")
{
	const char * const text = "a";

	auto parser = rex::parse(text + 0, text + 3);

	SECTION("")
	{
		const auto result = parser.match(!(class_a | class_b));
		CHECK_FALSE(result.first);
		CHECK(result.second == text + 0);
	}

	SECTION("")
	{
		const auto result = parser.match(!(class_b | class_a));
		CHECK_FALSE(result.first);
		CHECK(result.second == text + 0);
	}

	SECTION("")
	{
		const auto result = parser.match(!(class_b | class_c));
		CHECK(result.first);
		CHECK(result.second == text + 1);
	}
}
