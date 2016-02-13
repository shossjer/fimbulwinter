
#include "catch.hpp"

#include <core/maths/Matrix.hpp>

namespace
{
	template <typename T>
	void require(const core::maths::Matrix<T> & matrix, const std::array<T, 4 * 4> & array)
	{
#if __GNUG__
// stops GCC from shitting all over my terminal -- shossjer
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wparentheses"
#endif /* __GNUG__ */
		// TODO: make aligned
		typename core::maths::Matrix<T>::array_type aligned_buffer;
		matrix.get_aligned(aligned_buffer);

		  CHECK(aligned_buffer[ 0] == Approx(array[ 0]));
		  CHECK(aligned_buffer[ 4] == Approx(array[ 1]));
		  CHECK(aligned_buffer[ 8] == Approx(array[ 2]));
		  CHECK(aligned_buffer[12] == Approx(array[ 3]));
		  CHECK(aligned_buffer[ 1] == Approx(array[ 4]));
		  CHECK(aligned_buffer[ 5] == Approx(array[ 5]));
		  CHECK(aligned_buffer[ 9] == Approx(array[ 6]));
		  CHECK(aligned_buffer[13] == Approx(array[ 7]));
		  CHECK(aligned_buffer[ 2] == Approx(array[ 8]));
		  CHECK(aligned_buffer[ 6] == Approx(array[ 9]));
		  CHECK(aligned_buffer[10] == Approx(array[10]));
		  CHECK(aligned_buffer[14] == Approx(array[11]));
		  CHECK(aligned_buffer[ 3] == Approx(array[12]));
		  CHECK(aligned_buffer[ 7] == Approx(array[13]));
		  CHECK(aligned_buffer[11] == Approx(array[14]));
		REQUIRE(aligned_buffer[15] == Approx(array[15]));
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

TEST_CASE( "matrix default construction", "[maths]" )
{
	core::maths::Matrixf matrix;
}

TEST_CASE( "matrix copy construction", "[maths]" )
{
	core::maths::Matrixf matrix1{  1,  -2,   3,  -4,
	                              -5,   6,  -7,   8,
	                               9, -10,  11, -12,
	                             -13,  14, -15,  16};
	core::maths::Matrixf matrix2{matrix1};
	require(matrix2,
	        {  1,  -2,   3,  -4,
	          -5,   6,  -7,   8,
	           9, -10,  11, -12,
	         -13,  14, -15,  16});
}

TEST_CASE( "matrix construction", "[maths]" )
{
	core::maths::Matrixf matrix{  1,  -2,   3,  -4,
	                             -5,   6,  -7,   8,
	                              9, -10,  11, -12,
	                            -13,  14, -15,  16};
	require(matrix,
	        {  1,  -2,   3,  -4,
	          -5,   6,  -7,   8,
	           9, -10,  11, -12,
	         -13,  14, -15,  16});
}

TEST_CASE( "matrix copy assignment", "[maths]")
{
	core::maths::Matrixf matrix;
	matrix = core::maths::Matrixf{  1,  -2,   3,  -4,
	                               -5,   6,  -7,   8,
	                                9, -10,  11, -12,
	                              -13,  14, -15,  16};
	require(matrix,
	        {  1,  -2,   3,  -4,
	          -5,   6,  -7,   8,
	           9, -10,  11, -12,
	         -13,  14, -15,  16});
}

////////////////////////////////////////////////////////////////////////////////
//
//  arithmetics
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix multiplication", "[maths]" )
{
	core::maths::Matrixf matrix1{  1,  -2,   3,  -4,
	                              -5,   6,  -7,   8,
	                               9, -10,  11, -12,
	                             -13,  14, -15,  16};
	core::maths::Matrixf matrix2{  2,   3,   5,   7,
	                              11,  13,  17,  19,
	                              23,  29,  31,  37,
	                              41,  43,  47,  53};
	require(matrix1 * matrix2,
	        {-115, -108, -124, -132,
	          223,  204,  236,  244,
	         -331, -300, -348, -356,
	          439,  396,  460,  468});
	require(matrix1 *= matrix2,
	        {-115, -108, -124, -132,
	          223,  204,  236,  244,
	         -331, -300, -348, -356,
	          439,  396,  460,  468});
}

////////////////////////////////////////////////////////////////////////////////
//
//  member methods
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix set", "[maths]")
{
	core::maths::Matrixf matrix;
	matrix.set( 1,  2,  3,  4,
	            5,  6,  7,  8,
	            9, 10, 11, 12,
	           13, 14, 15, 16);
	require(matrix,
	        { 1,  2,  3,  4,
	          5,  6,  7,  8,
	          9, 10, 11, 12,
	         13, 14, 15, 16});

	core::maths::Matrixf::array_type buffer = { 1, -5,   9, -13,
	                                           -2,  6, -10,  14,
	                                            3, -7,  11, -15,
	                                           -4,  8, -12,  16};
	matrix.set(buffer);
	require(matrix,
	        {  1,  -2,   3,  -4,
	          -5,   6,  -7,   8,
	           9, -10,  11, -12,
	         -13,  14, -15,  16});

	// TODO: make aligned
	core::maths::Matrixf::array_type aligned_buffer = {2, 11, 23, 41,
	                                                   3, 13, 29, 43,
	                                                   5, 17, 31, 47,
	                                                   7, 19, 37, 53};
	matrix.set_aligned(aligned_buffer);
	require(matrix,
	        {  2,  3,  5,  7,
	          11, 13, 17, 19,
	          23, 29, 31, 37,
	          41, 43, 47, 53});
}

////////////////////////////////////////////////////////////////////////////////
//
//  static methods
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "matrix identity", "[maths]" )
{
	require(core::maths::Matrixf::identity(),
	        {1, 0, 0, 0,
	         0, 1, 0, 0,
	         0, 0, 1, 0,
	         0, 0, 0, 1});
}

TEST_CASE( "matrix ortho", "[maths]" )
{
	require(core::maths::Matrixf::ortho(-2, 3, -5, 7, -11, 13),
	        {0.4f, 0       ,  0       , -0.2f    ,
	         0   , 0.16667f,  0       , -0.16667f,
	         0   , 0       , -0.08333f, -0.08333f,
	         0   , 0       ,  0       ,  1       });
}

TEST_CASE( "matrix perspective", "[maths]" )
{
	require(core::maths::Matrixf::perspective(core::maths::make_degree(90.f), 4.f / 3.f, 1, 128),
	        {0.75f, 0  ,  0       ,  0       ,
	         0    , 1.f,  0       ,  0       ,
	         0    , 0  , -1.01575f, -2.01575f,
	         0    , 0  , -1       ,  0       });
}

TEST_CASE( "matrix rotation", "[maths]" )
{
	require(core::maths::Matrixf::rotation(core::maths::make_degree(90.f), 1, 1, 1),
	        { 0.33333f, -0.24402f,  0.91068f, 0,
	          0.91068f,  0.33333f, -0.24402f, 0,
	         -0.24402f,  0.91068f,  0.33333f, 0,
	          0       ,  0       ,  0       , 1});
}

TEST_CASE( "matrix translation", "[maths]" )
{
	require(core::maths::Matrixf::translation(2, 3, 5),
	        {1, 0, 0, 2,
	         0, 1, 0, 3,
	         0, 0, 1, 5,
	         0, 0, 0, 1});
}
