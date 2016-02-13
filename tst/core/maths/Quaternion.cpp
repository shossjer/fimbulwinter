
#include "catch.hpp"

#include <core/maths/Quaternion.hpp>

namespace
{
	template <typename T>
	void require(const core::maths::Quaternion<T> & quat, const std::array<T, 4> & array)
	{
#if __GNUG__
// stops GCC from shitting all over my terminal -- shossjer
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wparentheses"
#endif /* __GNUG__ */
		// TODO: make aligned
		typename core::maths::Quaternion<T>::array_type aligned_buffer;
		quat.get_aligned(aligned_buffer);

		  CHECK(aligned_buffer[0] == Approx(array[0]));
		  CHECK(aligned_buffer[1] == Approx(array[1]));
		  CHECK(aligned_buffer[2] == Approx(array[2]));
		REQUIRE(aligned_buffer[3] == Approx(array[3]));
#if __GNUG__
# pragma GCC diagnostic pop
#endif /* __GNUG__ */
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  construction/destruction
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "quaternion default construction", "[maths]" )
{
	using namespace core::maths;
	{
		Quaternionf quat1;
	}
	{
		Quaterniond quat1;
	}
}

TEST_CASE( "quaternion copy construction", "[maths]" )
{
	using namespace core::maths;
	{
		Quaternionf quat1{2.f, 3.f, 5.f, 7.f};
		Quaternionf quat2{quat1};
		require(quat2, {2.f, 3.f, 5.f, 7.f});
	}
	{
		Quaterniond quat1{2., 3., 5., 7.};
		Quaterniond quat2{quat1};
		require(quat2, {2., 3., 5., 7.});
	}
}

TEST_CASE( "quaternion construction", "[maths]" )
{
	using namespace core::maths;
	{
		Quaternionf quat1{2.f, 3.f, 5.f, 7.f};
		require(quat1, {2.f, 3.f, 5.f, 7.f});
	}
	{
		Quaterniond quat1{2., 3., 5., 7.};
		require(quat1, {2., 3., 5., 7.});
	}
}

TEST_CASE( "quaternion copy assignment", "[maths]" )
{
	using namespace core::maths;
	{
		Quaternionf quat1;
		quat1 = Quaternionf{2.f, 3.f, 5.f, 7.f};
		require(quat1, {2.f, 3.f, 5.f, 7.f});
	}
	{
		Quaternionf quat1;
		quat1 = Quaternionf{2., 3., 5., 7.};
		require(quat1, {2., 3., 5., 7.});
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  arithmetics
//
//////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//  member methods
//
//////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//  static methods
//
//////////////////////////////////////////////////////////////////////
