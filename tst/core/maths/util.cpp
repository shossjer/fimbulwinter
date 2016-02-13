
#include "catch.hpp"

#include <core/maths/util.hpp>

namespace
{
	constexpr float quarter_deg_f = 90.f;
	constexpr double quarter_deg_d = 90.;
	constexpr float quarter_rad_f = 3.141592653589793238462643383279502884f / 2.f;
	constexpr double quarter_rad_d = 3.141592653589793238462643383279502884 / 2.;
}

////////////////////////////////////////////////////////////////////////////////
//
//  degree
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "degree default construction", "[maths]" )
{
	{
		core::maths::degreef deg;
	}
	{
		core::maths::degreed deg;
	}
}

TEST_CASE( "degree copy construction", "[maths]" )
{
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::degreef deg2{deg1};
		REQUIRE(deg2.get() == Approx(quarter_deg_f));
	}
	{
		core::maths::degreed deg1{quarter_deg_d};
		core::maths::degreed deg2{deg1};
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::degreed deg2{deg1};
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	// this should not compile!
	// {
	// 	core::maths::degreed deg1{quarter_deg_d};
	// 	core::maths::degreef deg2{deg1}; // <-- here
	// 	REQUIRE(deg2.get() == Approx(quarter_deg_f));
	// }
}

TEST_CASE( "degree float/double conversion", "[maths]" )
{
	{
		core::maths::degreef deg = core::maths::make_degree(quarter_deg_f);
		REQUIRE(deg.get() == Approx(quarter_deg_f));
	}
	{
		core::maths::degreed deg = core::maths::make_degree(quarter_deg_d);
		REQUIRE(deg.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::degreef deg2 = core::maths::make_degree<float>(deg1);
		REQUIRE(deg2.get() == Approx(quarter_deg_f));
	}
	{
		core::maths::degreed deg1{quarter_deg_d};
		core::maths::degreed deg2 = core::maths::make_degree<double>(deg1);
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::degreed deg2 = core::maths::make_degree<double>(deg1);
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::degreed deg1{quarter_deg_d};
		core::maths::degreef deg2 = core::maths::make_degree<float>(deg1);
		REQUIRE(deg2.get() == Approx(quarter_deg_f));
	}
}

TEST_CASE( "degree radian conversion", "[maths]" )
{
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::radianf rad2{deg1};
		REQUIRE(rad2.get() == Approx(quarter_rad_f));
	}
	{
		core::maths::degreed deg1{quarter_deg_d};
		core::maths::radiand rad2{deg1};
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::radiand rad2{deg1};
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	// this should not compile!
	// {
	// 	core::maths::degreed deg1{quarter_deg_d};
	// 	core::maths::radianf rad2{deg1}; // <-- here
	// 	REQUIRE(rad2.get() == Approx(quarter_rad_f));
	// }
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::radianf rad2 = make_radian(deg1);
		REQUIRE(rad2.get() == Approx(quarter_rad_f));
	}
	{
		core::maths::degreed deg1{quarter_deg_d};
		core::maths::radiand rad2 = make_radian(deg1);
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::degreef deg1{quarter_deg_f};
		core::maths::radiand rad2 = make_radian(deg1);
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	// this should not compile!
	// {
	// 	core::maths::degreed deg1{quarter_deg_d};
	// 	core::maths::radianf rad2 = make_radian(deg1); // <-- here
	// 	REQUIRE(rad2.get() == Approx(quarter_rad_f));
	// }
}

////////////////////////////////////////////////////////////////////////////////
//
//  radian
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "radian default construction", "[maths]" )
{
	{
		core::maths::radianf rad;
	}
	{
		core::maths::radiand rad;
	}
}

TEST_CASE( "radian copy construction", "[maths]" )
{
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::radianf rad2{rad1};
		REQUIRE(rad2.get() == Approx(quarter_rad_f));
	}
	{
		core::maths::radiand rad1{quarter_rad_d};
		core::maths::radiand rad2{rad1};
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::radiand rad2{rad1};
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	// this should not compile!
	// {
	// 	core::maths::radiand rad1{quarter_rad_d};
	// 	core::maths::radianf rad2{rad1}; // <-- here
	// 	REQUIRE(rad2.get() == Approx(quarter_rad_f));
	// }
}

TEST_CASE( "radian float/double conversion", "[maths]" )
{
	{
		core::maths::radianf rad = core::maths::make_radian(quarter_rad_f);
		REQUIRE(rad.get() == Approx(quarter_rad_f));
	}
	{
		core::maths::radiand rad = core::maths::make_radian(quarter_rad_d);
		REQUIRE(rad.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::radianf rad2 = core::maths::make_radian<float>(rad1);
		REQUIRE(rad2.get() == Approx(quarter_rad_f));
	}
	{
		core::maths::radiand rad1{quarter_rad_d};
		core::maths::radiand rad2 = core::maths::make_radian<double>(rad1);
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::radiand rad2 = core::maths::make_radian<double>(rad1);
		REQUIRE(rad2.get() == Approx(quarter_rad_d));
	}
	{
		core::maths::radiand rad1{quarter_rad_d};
		core::maths::radianf rad2 = core::maths::make_radian<float>(rad1);
		REQUIRE(rad2.get() == Approx(quarter_rad_f));
	}
}

TEST_CASE( "radian degree conversion", "[maths]" )
{
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::degreef deg2{rad1};
		REQUIRE(deg2.get() == Approx(quarter_deg_f));
	}
	{
		core::maths::radiand rad1{quarter_rad_d};
		core::maths::degreed deg2{rad1};
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::degreed deg2{rad1};
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	// this should not compile!
	// {
	// 	core::maths::radiand rad1{quarter_rad_d};
	// 	core::maths::degreef deg2{rad1}; // <-- here
	// 	REQUIRE(deg2.get() == Approx(quarter_deg_f));
	// }
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::degreef deg2 = make_degree(rad1);
		REQUIRE(deg2.get() == Approx(quarter_deg_f));
	}
	{
		core::maths::radiand rad1{quarter_rad_d};
		core::maths::degreed deg2 = make_degree(rad1);
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	{
		core::maths::radianf rad1{quarter_rad_f};
		core::maths::degreed deg2 = make_degree(rad1);
		REQUIRE(deg2.get() == Approx(quarter_deg_d));
	}
	// this should not compile!
	// {
	// 	core::maths::radiand rad1{quarter_rad_d};
	// 	core::maths::degreef deg2 = make_degree(rad1); // <-- here
	// 	REQUIRE(deg2.get() == Approx(quarter_deg_f));
	// }
}
