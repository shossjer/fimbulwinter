#include "core/maths/Matrix.hpp"

#include <catch2/catch.hpp>

namespace
{
	template <std::size_t M, std::size_t N, typename T>
	void require(const core::maths::Matrix<M, N, T> & matrix, const std::array<T, M * N> & array)
	{
#if __GNUG__
// stops GCC from shitting all over my terminal -- shossjer
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wparentheses"
#endif /* __GNUG__ */
		// TODO: make aligned
		typename core::maths::Matrix<M, N, T>::array_type aligned_buffer;
		matrix.get_aligned(aligned_buffer);

		  CHECK(aligned_buffer[ 0] == Approx(array[ 0]).epsilon(0.001));
		  CHECK(aligned_buffer[ 4] == Approx(array[ 1]).epsilon(0.001));
		  CHECK(aligned_buffer[ 8] == Approx(array[ 2]).epsilon(0.001));
		  CHECK(aligned_buffer[12] == Approx(array[ 3]).epsilon(0.001));
		  CHECK(aligned_buffer[ 1] == Approx(array[ 4]).epsilon(0.001));
		  CHECK(aligned_buffer[ 5] == Approx(array[ 5]).epsilon(0.001));
		  CHECK(aligned_buffer[ 9] == Approx(array[ 6]).epsilon(0.001));
		  CHECK(aligned_buffer[13] == Approx(array[ 7]).epsilon(0.001));
		  CHECK(aligned_buffer[ 2] == Approx(array[ 8]).epsilon(0.001));
		  CHECK(aligned_buffer[ 6] == Approx(array[ 9]).epsilon(0.001));
		  CHECK(aligned_buffer[10] == Approx(array[10]).epsilon(0.001));
		  CHECK(aligned_buffer[14] == Approx(array[11]).epsilon(0.001));
		  CHECK(aligned_buffer[ 3] == Approx(array[12]).epsilon(0.001));
		  CHECK(aligned_buffer[ 7] == Approx(array[13]).epsilon(0.001));
		  CHECK(aligned_buffer[11] == Approx(array[14]).epsilon(0.001));
		REQUIRE(aligned_buffer[15] == Approx(array[15]).epsilon(0.001));
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

TEST_CASE( "matrix default construction", "[core][maths]" )
{
	core::maths::Matrix4x4f matrix;
	fiw_unused(matrix);
}

TEST_CASE( "matrix copy construction", "[core][maths]" )
{
	core::maths::Matrix4x4f matrix1{  1.f,  -2.f,   3.f,  -4.f,
	                                 -5.f,   6.f,  -7.f,   8.f,
	                                  9.f, -10.f,  11.f, -12.f,
	                                -13.f,  14.f, -15.f,  16.f};
	core::maths::Matrix4x4f matrix2{matrix1};
	require(matrix2,
	        {{  1.f,  -2.f,   3.f,  -4.f,
	           -5.f,   6.f,  -7.f,   8.f,
	            9.f, -10.f,  11.f, -12.f,
	          -13.f,  14.f, -15.f,  16.f}});
}

TEST_CASE( "matrix construction", "[core][maths]" )
{
	core::maths::Matrix4x4f matrix{  1.f,  -2.f,   3.f,  -4.f,
	                                -5.f,   6.f,  -7.f,   8.f,
	                                 9.f, -10.f,  11.f, -12.f,
	                               -13.f,  14.f, -15.f,  16.f};
	require(matrix,
	        {{  1.f,  -2.f,   3.f,  -4.f,
	           -5.f,   6.f,  -7.f,   8.f,
	            9.f, -10.f,  11.f, -12.f,
	          -13.f,  14.f, -15.f,  16.f}});
}

TEST_CASE( "matrix copy assignment", "[core][maths]")
{
	core::maths::Matrix4x4f matrix;
	matrix = core::maths::Matrix4x4f{  1.f,  -2.f,   3.f,  -4.f,
	                                  -5.f,   6.f,  -7.f,   8.f,
	                                   9.f, -10.f,  11.f, -12.f,
	                                 -13.f,  14.f, -15.f,  16.f};
	require(matrix,
	        {{  1.f,  -2.f,   3.f,  -4.f,
	           -5.f,   6.f,  -7.f,   8.f,
	            9.f, -10.f,  11.f, -12.f,
	          -13.f,  14.f, -15.f,  16.f}});
}

////////////////////////////////////////////////////////////////////////////////
//
//  arithmetics
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix multiplication", "[core][maths]" )
{
	core::maths::Matrix4x4f matrix1{  1.f,  -2.f,   3.f,  -4.f,
	                                 -5.f,   6.f,  -7.f,   8.f,
	                                  9.f, -10.f,  11.f, -12.f,
	                                -13.f,  14.f, -15.f,  16.f};
	core::maths::Matrix4x4f matrix2{  2.f,   3.f,   5.f,   7.f,
	                                 11.f,  13.f,  17.f,  19.f,
	                                 23.f,  29.f,  31.f,  37.f,
	                                 41.f,  43.f,  47.f,  53.f};
	require(matrix1 * matrix2,
	        {{-115.f, -108.f, -124.f, -132.f,
	           223.f,  204.f,  236.f,  244.f,
	          -331.f, -300.f, -348.f, -356.f,
	           439.f,  396.f,  460.f,  468.f}});
	require(matrix1 *= matrix2,
	        {{-115.f, -108.f, -124.f, -132.f,
	           223.f,  204.f,  236.f,  244.f,
	          -331.f, -300.f, -348.f, -356.f,
	           439.f,  396.f,  460.f,  468.f}});
}

////////////////////////////////////////////////////////////////////////////////
//
//  member methods
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix set", "[core][maths]")
{
	core::maths::Matrix4x4f matrix;
	matrix.set( 1.f,  2.f,  3.f,  4.f,
	            5.f,  6.f,  7.f,  8.f,
	            9.f, 10.f, 11.f, 12.f,
	           13.f, 14.f, 15.f, 16.f);
	require(matrix,
	        {{ 1.f,  2.f,  3.f,  4.f,
	           5.f,  6.f,  7.f,  8.f,
	           9.f, 10.f, 11.f, 12.f,
	          13.f, 14.f, 15.f, 16.f}});

	core::maths::Matrix4x4f::array_type buffer = { 1.f, -5.f,   9.f, -13.f,
	                                              -2.f,  6.f, -10.f,  14.f,
	                                               3.f, -7.f,  11.f, -15.f,
	                                              -4.f,  8.f, -12.f,  16.f};
	matrix.set(buffer);
	require(matrix,
	        {{  1.f,  -2.f,   3.f,  -4.f,
	           -5.f,   6.f,  -7.f,   8.f,
	            9.f, -10.f,  11.f, -12.f,
	          -13.f,  14.f, -15.f,  16.f}});

	// TODO: make aligned
	core::maths::Matrix4x4f::array_type aligned_buffer = {2.f, 11.f, 23.f, 41.f,
	                                                      3.f, 13.f, 29.f, 43.f,
	                                                      5.f, 17.f, 31.f, 47.f,
	                                                      7.f, 19.f, 37.f, 53.f};
	matrix.set_aligned(aligned_buffer);
	require(matrix,
	        {{  2.f,  3.f,  5.f,  7.f,
	           11.f, 13.f, 17.f, 19.f,
	           23.f, 29.f, 31.f, 37.f,
	           41.f, 43.f, 47.f, 53.f}});
}

////////////////////////////////////////////////////////////////////////////////
//
//  static methods
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix identity", "[core][maths]" )
{
	require(core::maths::Matrix4x4f::identity(),
	        {{1.f, 0.f, 0.f, 0.f,
	          0.f, 1.f, 0.f, 0.f,
	          0.f, 0.f, 1.f, 0.f,
	          0.f, 0.f, 0.f, 1.f}});
}

TEST_CASE( "matrix ortho", "[core][maths]" )
{
	require(core::maths::Matrix4x4f::ortho(-2, 3, -5, 7, -11, 13),
	        {{0.4f, 0.f     ,  0.f     , -0.2f    ,
	          0.f , 0.16667f,  0.f     , -0.16667f,
	          0.f , 0.f     , -0.08333f, -0.08333f,
	          0.f , 0.f     ,  0.f     ,  1.f     }});
}

TEST_CASE( "matrix perspective", "[core][maths]" )
{
	require(core::maths::Matrix4x4f::perspective(core::maths::make_degree(90.f), 4.f / 3.f, 1, 128),
	        {{0.75f, 0.f,  0.f     ,  0.f     ,
	          0.f  , 1.f,  0.f     ,  0.f     ,
	          0.f  , 0.f, -1.01575f, -2.01575f,
	          0.f  , 0.f, -1.f     ,  0.f     }});
}

TEST_CASE( "matrix rotation", "[core][maths]" )
{
	require(core::maths::Matrix4x4f::rotation(core::maths::make_degree(90.f), 1, 1, 1),
	        {{ 0.33333f, -0.24402f,  0.91068f, 0.f,
	           0.91068f,  0.33333f, -0.24402f, 0.f,
	          -0.24402f,  0.91068f,  0.33333f, 0.f,
	           0.f     ,  0.f     ,  0.f     , 1.f}});
}

TEST_CASE( "matrix translation", "[core][maths]" )
{
	require(core::maths::Matrix4x4f::translation(2, 3, 5),
	        {{1.f, 0.f, 0.f, 2.f,
	          0.f, 1.f, 0.f, 3.f,
	          0.f, 0.f, 1.f, 5.f,
	          0.f, 0.f, 0.f, 1.f}});
}
