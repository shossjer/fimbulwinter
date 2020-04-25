#include "utility/compound.hpp"

#include <catch/catch.hpp>

#include <string>

TEST_CASE("compound singles", "[utility][compound]")
{
	std::string s = "1";

	SECTION("of value type")
	{
		utility::compound<std::string> c(s);

		SECTION("can be assigned to")
		{
			std::string ss = "22";

			SECTION("by lvalues")
			{
				c = ss;
				CHECK(s == "1");
				CHECK(ss == "22");
			}

			SECTION("by rvalues")
			{
				c = std::move(ss);
				CHECK(s == "1");
				CHECK(ss == "");
			}
		}

		SECTION("can convert to value")
		{
			std::string t = c;
			CHECK(t == "1");
			CHECK(s == "1");
		}

		SECTION("can convert to lvalue reference")
		{
			std::string & t = c;
			CHECK(t == "1");
			CHECK(s == "1");
		}

		SECTION("can convert to rvalue reference")
		{
			std::string && t = std::move(c);
			CHECK(t == "1");
			CHECK(s == "1");
		}
	}

	SECTION("of lvalue reference type")
	{
		utility::compound<std::string &> c(s);

		SECTION("can be assigned to")
		{
			std::string ss = "22";

			SECTION("by lvalues")
			{
				c = ss;
				CHECK(s == "22");
				CHECK(ss == "22");
			}

			SECTION("by rvalues")
			{
				c = std::move(ss);
				CHECK(s == "22");
				CHECK(ss == "");
			}
		}

		SECTION("can convert to value")
		{
			std::string t = c;
			CHECK(t == "1");
			CHECK(s == "1");
		}

		SECTION("can convert to lvalue reference")
		{
			std::string & t = c;
			CHECK(t == "1");
			CHECK(s == "1");
			CHECK(&t == &s);
		}

		SECTION("can convert to lvalue reference (after move)")
		{
			std::string & t = std::move(c);
			CHECK(t == "1");
			CHECK(s == "1");
			CHECK(&t == &s);
		}
	}

	SECTION("of rvalue reference type")
	{
		utility::compound<std::string &&> c(std::move(s));

		SECTION("can be assigned to")
		{
			std::string ss = "22";

			SECTION("by lvalues")
			{
				c = ss;
				CHECK(s == "22");
				CHECK(ss == "22");
			}

			SECTION("by rvalues")
			{
				c = std::move(ss);
				CHECK(s == "22");
				CHECK(ss == "");
			}
		}

		SECTION("can convert to value")
		{
			std::string t = c;
			CHECK(t == "1");
			CHECK(s == "1");
		}

		SECTION("can convert to lvalue reference")
		{
			std::string & t = c;
			CHECK(t == "1");
			CHECK(s == "1");
			CHECK(&t == &s);
		}

		SECTION("can convert to rvalue reference")
		{
			std::string && t = std::move(c);
			CHECK(t == "1");
			CHECK(s == "1");
			CHECK(&t == &s);
		}
	}
}

TEST_CASE("compound pairs", "[utility][compound]")
{
	std::string s = "1";
	std::string ss = "22";

	SECTION("of reference types")
	{
		utility::compound<std::string &, std::string &&> c(s, std::move(ss));

		SECTION("can be assigned to")
		{
			std::string sss = "333";
			std::string ssss = "4444";

			SECTION("with pairs")
			{
				c = std::pair<std::string &, std::string &&>(sss, std::move(ssss));
				CHECK(s == "333");
				CHECK(ss == "4444");
				CHECK(sss == "333");
				CHECK(ssss == "");
			}

			SECTION("with tuples")
			{
				c = std::tuple<std::string &, std::string &&>(sss, std::move(ssss));
				CHECK(s == "333");
				CHECK(ss == "4444");
				CHECK(sss == "333");
				CHECK(ssss == "");
			}

			SECTION("by member access")
			{
				c.first = sss;
				c.second = ssss;
				CHECK(s == "333");
				CHECK(ss == "4444");
				CHECK(sss == "333");
				CHECK(ssss == "4444");
			}
		}

		SECTION("can convert to value")
		{
			std::pair<std::string, std::string> p = c;
			CHECK(s == "1");
			CHECK(ss == "22");
		}

		SECTION("can convert to rvalue reference")
		{
			std::pair<std::string, std::string> && p = std::move(c);
			CHECK(s == "1");
			CHECK(ss == "");
		}

		SECTION("can convert to proxy value")
		{
			std::pair<std::string &, std::string &> p = c;
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(&p.first == &s);
			CHECK(&p.second == &ss);
		}

		SECTION("can convert to proxy lvalue reference")
		{
			std::pair<std::string &, std::string &&> & p = c;
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(&p.first == &s);
			CHECK(&p.second == &ss);
		}

		SECTION("can convert to proxy rvalue reference")
		{
			std::pair<std::string &, std::string &&> && p = std::move(c);
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(&p.first == &s);
			CHECK(&p.second == &ss);
		}
	}
}

TEST_CASE("compound tuples", "[utility][compound]")
{
	std::string s = "1";
	std::string ss = "22";
	std::string sss = "333";

	SECTION("of value and reference types")
	{
		utility::compound<std::string, std::string &, std::string &&> c(s, ss, std::move(sss));

		SECTION("can be assigned to")
		{
			std::string ssss = "4444";
			std::string sssss = "55555";
			std::string ssssss = "666666";

			SECTION("with tuples")
			{
				c = std::tuple<std::string, std::string &, std::string &&>(ssss, sssss, std::move(ssssss));
				CHECK(s == "1");
				CHECK(ss == "55555");
				CHECK(sss == "666666");
				CHECK(ssss == "4444");
				CHECK(sssss == "55555");
				CHECK(ssssss == "");
			}

			SECTION("by get access")
			{
				std::get<0>(c) = ssss;
				std::get<1>(c) = sssss;
				std::get<2>(c) = ssssss;
				CHECK(s == "1");
				CHECK(ss == "55555");
				CHECK(sss == "666666");
				CHECK(ssss == "4444");
				CHECK(sssss == "55555");
				CHECK(ssssss == "666666");
			}
		}

		SECTION("can convert to value")
		{
			std::tuple<std::string, std::string, std::string> t = c;
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(sss == "333");
		}

		SECTION("can convert to rvalue reference")
		{
			std::tuple<std::string, std::string, std::string> && t = std::move(c);
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(sss == "");
		}

		SECTION("can convert to proxy value")
		{
			std::tuple<std::string, std::string &, std::string &> t = c;
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(sss == "333");
			CHECK(&std::get<1>(t) == &ss);
			CHECK(&std::get<2>(t) == &sss);
		}

		SECTION("can convert to proxy lvalue reference")
		{
			std::tuple<std::string, std::string &, std::string &&> & t = c;
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(sss == "333");
			CHECK(&std::get<1>(t) == &ss);
			CHECK(&std::get<2>(t) == &sss);
		}

		SECTION("can convert to proxy rvalue reference")
		{
			std::tuple<std::string, std::string &, std::string &&> && t = std::move(c);
			CHECK(s == "1");
			CHECK(ss == "22");
			CHECK(sss == "333");
			CHECK(&std::get<1>(t) == &ss);
			CHECK(&std::get<2>(t) == &sss);
		}
	}
}
