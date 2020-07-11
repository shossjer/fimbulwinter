#include "core/maths/Vector.hpp"

#include "utility/type_traits.hpp"

#include <catch2/catch.hpp>

namespace
{
	template <typename T, std::size_t N>
	void require(const core::maths::Vector<N, T> & vector, const std::array<T, N> & array)
	{
#if __GNUG__
// stops GCC from shitting all over my terminal -- shossjer
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wparentheses"
#endif /* __GNUG__ */
		// TODO: make aligned
		typename core::maths::Vector<N, T>::array_type aligned_buffer;
		vector.get_aligned(aligned_buffer);

		for (std::size_t i = 0; i < N - 1; i++)
		{
			CHECK(aligned_buffer[i] == Approx(array[i]));
		}
		REQUIRE(aligned_buffer[N - 1] == Approx(array[N - 1]));
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

TEST_CASE( "vector default construction", "[maths]" )
{
	{
		core::maths::Vector2f vec;
	}
	{
		core::maths::Vector3f vec;
	}
	{
		core::maths::Vector4f vec;
	}
	{
		core::maths::Vector2d vec;
	}
	{
		core::maths::Vector3d vec;
	}
	{
		core::maths::Vector4d vec;
	}
}

TEST_CASE( "vector copy construction", "[maths]" )
{
	{
		core::maths::Vector2f vec1{1.f, 2.f};
		core::maths::Vector2f vec2{vec1};
		require(vec2, {{1, 2}});
	}
	{
		core::maths::Vector3f vec1{1.f, 2.f, 3.f};
		core::maths::Vector3f vec2{vec1};
		require(vec2, {{1, 2, 3}});
	}
	{
		core::maths::Vector4f vec1{1.f, 2.f, 3.f, 4.f};
		core::maths::Vector4f vec2{vec1};
		require(vec2, {{1, 2, 3, 4}});
	}
	{
		core::maths::Vector2d vec1{1., 2.};
		core::maths::Vector2d vec2{vec1};
		require(vec2, {{1, 2}});
	}
	{
		core::maths::Vector3d vec1{1., 2., 3.};
		core::maths::Vector3d vec2{vec1};
		require(vec2, {{1, 2, 3}});
	}
	{
		core::maths::Vector4d vec1{1., 2., 3., 4.};
		core::maths::Vector4d vec2{vec1};
		require(vec2, {{1, 2, 3, 4}});
	}
}

TEST_CASE( "vector construction", "[maths]" )
{
	{
		core::maths::Vector2f vec{1.f, 2.f};
		require(vec, {{1, 2}});
	}
	{
		core::maths::Vector3f vec{1.f, 2.f, 3.f};
		require(vec, {{1, 2, 3}});
	}
	{
		core::maths::Vector4f vec{1.f, 2.f, 3.f, 4.f};
		require(vec, {{1, 2, 3, 4}});
	}
	{
		core::maths::Vector2d vec{1., 2.};
		require(vec, {{1, 2}});
	}
	{
		core::maths::Vector3d vec{1., 2., 3.};
		require(vec, {{1, 2, 3}});
	}
	{
		core::maths::Vector4d vec{1., 2., 3., 4.};
		require(vec, {{1, 2, 3, 4}});
	}
}

TEST_CASE( "vector copy assignment", "[maths]" )
{
	{
		core::maths::Vector2f vec;
		vec = core::maths::Vector2f{1.f, 2.f};
		require(vec, {{1, 2}});
	}
	{
		core::maths::Vector3f vec;
		vec = core::maths::Vector3f{1.f, 2.f, 3.f};
		require(vec, {{1, 2, 3}});
	}
	{
		core::maths::Vector4f vec;
		vec = core::maths::Vector4f{1.f, 2.f, 3.f, 4.f};
		require(vec, {{1, 2, 3, 4}});
	}
	{
		core::maths::Vector2d vec;
		vec = core::maths::Vector2d{1., 2.};
		require(vec, {{1, 2}});
	}
	{
		core::maths::Vector3d vec;
		vec = core::maths::Vector3d{1., 2., 3.};
		require(vec, {{1, 2, 3}});
	}
	{
		core::maths::Vector4d vec;
		vec = core::maths::Vector4d{1., 2., 3., 4.};
		require(vec, {{1, 2, 3, 4}});
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  arithmetics
//
//////////////////////////////////////////////////////////////////////

namespace
{
	template <typename T, std::size_t N, int ...N1s, int ...N2s, int ...N3s>
	void vector_addition(mpl::integral_sequence<int, N1s...>,
	                     mpl::integral_sequence<int, N2s...>,
	                     mpl::integral_sequence<int, N3s...>)
	{
		core::maths::Vector<N, T> vec1{T(N1s)...};
		core::maths::Vector<N, T> vec2{T(N2s)...};

		require(vec1 + vec2, {{T(N3s)...}});

		vec1 += vec2;
		require(vec1, {{T(N3s)...}});
	}
	template <typename T, std::size_t N>
	void vector_addition()
	{
		using S1 = mpl::integral_sequence<int, 1, 2, 3,  4>;
		using S2 = mpl::integral_sequence<int, 2, 3, 5,  7>;
		using S3 = mpl::integral_sequence<int, 3, 5, 8, 11>;

		vector_addition<T, N>(mpl::integral_take<int, S1, N>{},
		                      mpl::integral_take<int, S2, N>{},
		                      mpl::integral_take<int, S3, N>{});
	}
}
TEST_CASE( "vector addition", "[maths]" )
{
	vector_addition<float, 2>();
	vector_addition<float, 3>();
	vector_addition<float, 4>();
	vector_addition<double, 2>();
	vector_addition<double, 3>();
	vector_addition<double, 4>();
}

namespace
{
	template <typename T, std::size_t N, int ...N1s, int ...N2s, int ...N3s>
	void vector_subtraction(mpl::integral_sequence<int, N1s...>,
	                        mpl::integral_sequence<int, N2s...>,
	                        mpl::integral_sequence<int, N3s...>)
	{
		core::maths::Vector<N, T> vec1{T(N1s)...};
		core::maths::Vector<N, T> vec2{T(N2s)...};

		require(vec1 - vec2, {{T(N3s)...}});

		vec1 -= vec2;
		require(vec1, {{T(N3s)...}});
	}
	template <typename T, std::size_t N>
	void vector_subtraction()
	{
		using S1 = mpl::integral_sequence<int,  1,  2,  3,  4>;
		using S2 = mpl::integral_sequence<int,  2,  3,  5,  7>;
		using S3 = mpl::integral_sequence<int, -1, -1, -2, -3>;

		vector_subtraction<T, N>(mpl::integral_take<int, S1, N>{},
		                         mpl::integral_take<int, S2, N>{},
		                         mpl::integral_take<int, S3, N>{});
	}
}
TEST_CASE( "vector subtraction", "[maths]" )
{
	vector_subtraction<float, 2>();
	vector_subtraction<float, 3>();
	vector_subtraction<float, 4>();
	vector_subtraction<double, 2>();
	vector_subtraction<double, 3>();
	vector_subtraction<double, 4>();
}

namespace
{
	template <typename T, std::size_t N, int ...N1s, int ...N2s, int ...N3s, int ...N4s>
	void vector_multiplication(mpl::integral_sequence<int, N1s...>,
	                           mpl::integral_sequence<int, N2s...>,
	                           mpl::integral_sequence<int, N3s...>,
	                           mpl::integral_sequence<int, N4s...>)
	{
		core::maths::Vector<N, T> vec1{T(N1s)...};

		require(vec1 * T(2), {{T(N2s)...}});
		require(T(3) * vec1, {{T(N3s)...}});

		vec1 *= T(5);
		require(vec1, {{T(N4s)...}});
	}
	template <typename T, std::size_t N>
	void vector_multiplication()
	{
		using S1 = mpl::integral_sequence<int, 1,  2,  3,  4>;
		using S2 = mpl::integral_sequence<int, 2,  4,  6,  8>;
		using S3 = mpl::integral_sequence<int, 3,  6,  9, 12>;
		using S4 = mpl::integral_sequence<int, 5, 10, 15, 20>;

		vector_multiplication<T, N>(mpl::integral_take<int, S1, N>{},
		                            mpl::integral_take<int, S2, N>{},
		                            mpl::integral_take<int, S3, N>{},
		                            mpl::integral_take<int, S4, N>{});
	}
}
TEST_CASE( "vector multiplication", "[maths]" )
{
	vector_multiplication<float, 2>();
	vector_multiplication<float, 3>();
	vector_multiplication<float, 4>();
	vector_multiplication<double, 2>();
	vector_multiplication<double, 3>();
	vector_multiplication<double, 4>();
}

namespace
{
	template <typename T, std::size_t N, int ...N1s, int ...N2s, int ...N3s, int ...N4s>
	void vector_division(mpl::integral_sequence<int, N1s...>,
	                     mpl::integral_sequence<int, N2s...>,
	                     mpl::integral_sequence<int, N3s...>,
	                     mpl::integral_sequence<int, N4s...>)
	{
		core::maths::Vector<N, T> vec1{T(N1s)...};

		require(vec1 / T(2), {{T(N2s)...}});
		// require(T(3) / vec1, {T(N3s)...}); // TODO: make it compile

		vec1 /= T(5);
		require(vec1, {{T(N4s)...}});
	}
	template <typename T, std::size_t N>
	void vector_division()
	{
		using S1 = mpl::integral_sequence<int, 10, 20, 30, 40>;
		using S2 = mpl::integral_sequence<int,  5, 10, 15, 20>;
		using S3 = mpl::integral_sequence<int, -1, -1, -1, -1>;
		using S4 = mpl::integral_sequence<int,  2,  4,  6,  8>;

		vector_division<T, N>(mpl::integral_take<int, S1, N>{},
		                      mpl::integral_take<int, S2, N>{},
		                      mpl::integral_take<int, S3, N>{},
		                      mpl::integral_take<int, S4, N>{});
	}
}
TEST_CASE( "vector division", "[maths]" )
{
	vector_division<float, 2>();
	vector_division<float, 3>();
	vector_division<float, 4>();
	vector_division<double, 2>();
	vector_division<double, 3>();
	vector_division<double, 4>();
}

////////////////////////////////////////////////////////////////////////////////
//
//  member methods
//
//////////////////////////////////////////////////////////////////////

namespace
{
	template <typename T, std::size_t N, int ...N1s, int ...N2s, int ...N3s>
	void vector_set(mpl::integral_sequence<int, N1s...>,
	                mpl::integral_sequence<int, N2s...>,
	                mpl::integral_sequence<int, N3s...>)
	{
		core::maths::Vector<N, T> vec;
		vec.set(T(N1s)...);
		require(vec, {{N1s...}});

		typename core::maths::Vector<N, T>::array_type buffer = {N2s...};
		vec.set(buffer);
		require(vec, {{N2s...}});

		// TODO: make aligned
		typename core::maths::Vector<N, T>::array_type aligned_buffer = {N3s...};
		vec.set_aligned(aligned_buffer);
		require(vec, {{N3s...}});
	}
	template <typename T, std::size_t N>
	void vector_set()
	{
		using S1 = mpl::integral_sequence<int,  1,  2,  3,  4>;
		using S2 = mpl::integral_sequence<int, -1, -2, -3, -4>;
		using S3 = mpl::integral_sequence<int,  2,  3,  5,  7>;

		vector_set<T, N>(mpl::integral_take<int, S1, N>{},
		                 mpl::integral_take<int, S2, N>{},
		                 mpl::integral_take<int, S3, N>{});
	}
}
TEST_CASE( "vector set", "[maths]" )
{
	vector_set<float, 2>();
	vector_set<float, 3>();
	vector_set<float, 4>();
	vector_set<double, 2>();
	vector_set<double, 3>();
	vector_set<double, 4>();
}

////////////////////////////////////////////////////////////////////////////////
//
//  static methods
//
//////////////////////////////////////////////////////////////////////

TEST_CASE( "vector cross", "[maths]" )
{
	using namespace core::maths;

	require(cross(Vector3f{1.f, 0.f, 0.f}, Vector3f{0.f, 1.f, 0.f}), {{0.f, 0.f, 1.f}});
	require(cross(Vector3f{0.f, 1.f, 0.f}, Vector3f{0.f, 0.f, 1.f}), {{1.f, 0.f, 0.f}});
	require(cross(Vector3f{0.f, 0.f, 1.f}, Vector3f{1.f, 0.f, 0.f}), {{0.f, 1.f, 0.f}});

	require(cross(Vector3d{1., 0., 0.}, Vector3d{0., 1., 0.}), {{0., 0., 1.}});
	require(cross(Vector3d{0., 1., 0.}, Vector3d{0., 0., 1.}), {{1., 0., 0.}});
	require(cross(Vector3d{0., 0., 1.}, Vector3d{1., 0., 0.}), {{0., 1., 0.}});
}

TEST_CASE( "vector dot", "[maths]" )
{
	using namespace core::maths;

	REQUIRE(dot(Vector2f{1.f, 2.f}          , Vector2f{2.f, 3.f}          ).get() == Approx( 8.f));
	REQUIRE(dot(Vector3f{1.f, 2.f, 3.f}     , Vector3f{2.f, 3.f, 5.f}     ).get() == Approx(23.f));
	REQUIRE(dot(Vector4f{1.f, 2.f, 3.f, 4.f}, Vector4f{2.f, 3.f, 5.f, 7.f}).get() == Approx(51.f));

	REQUIRE(dot(Vector2d{1., 2.}        , Vector2d{2., 3.}        ).get() == Approx( 8.));
	REQUIRE(dot(Vector3d{1., 2., 3.}    , Vector3d{2., 3., 5.}    ).get() == Approx(23.));
	REQUIRE(dot(Vector4d{1., 2., 3., 4.}, Vector4d{2., 3., 5., 7.}).get() == Approx(51.));
}

TEST_CASE( "vector lerp", "[maths]" )
{
	using namespace core::maths;

	require(lerp(Vector2f{1.f, 2.f}          , Vector2f{2.f, 3.f}          , .5f), {{1.5f, 2.5f}});
	require(lerp(Vector3f{1.f, 2.f, 3.f}     , Vector3f{2.f, 3.f, 5.f}     , .5f), {{1.5f, 2.5f, 4.f}});
	require(lerp(Vector4f{1.f, 2.f, 3.f, 4.f}, Vector4f{2.f, 3.f, 5.f, 7.f}, .5f), {{1.5f, 2.5f, 4.f, 5.5f}});

	require(lerp(Vector2d{1., 2.}        , Vector2d{2., 3.}        , .5), {{1.5, 2.5}});
	require(lerp(Vector3d{1., 2., 3.}    , Vector3d{2., 3., 5.}    , .5), {{1.5, 2.5, 4.}});
	require(lerp(Vector4d{1., 2., 3., 4.}, Vector4d{2., 3., 5., 7.}, .5), {{1.5, 2.5, 4., 5.5}});
}
