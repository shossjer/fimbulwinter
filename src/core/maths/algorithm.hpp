
#ifndef CORE_MATHS_ALGORITHM_HPP
#define CORE_MATHS_ALGORITHM_HPP

#include <config.h>

#include <utility/algorithm.hpp>

#include <core/debug.hpp>

namespace core
{
	namespace maths
	{
		template <std::size_t, std::size_t, typename>
		class Matrix;
		template <std::size_t, typename>
		class Plane;
		template <typename>
		class Scalar;
		template <std::size_t, typename>
		class Vector;
	}
}

namespace core
{
	namespace maths
	{
		////////////////////////////////////////////////////////////////////////
		//
		// various functions
		//
		////////////////////////////////////////////////////////////////////////
		/**
		 * Returns the square of `x`.
		 */
		template <typename T>
		inline auto square(T && x) -> decltype(x * x)
		{
			return x * x;
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  friend hack
		//
		////////////////////////////////////////////////////////////////////////
		struct algorithm
		{
			template <std::size_t M, std::size_t N, typename T>
			void set_column_impl(Matrix<M, N, T> & m, std::size_t c, const Vector<M, T> & v)
			{
				debug_assert(c < N);
				for (std::size_t i = 0; i < M; i++)
				{
					m.values[M * c + i] = v.values[i];
				}
			}
		};

		////////////////////////////////////////////////////////////////////////
		//
		//  member functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t M, std::size_t N, typename T>
		inline void Matrix<M, N, T>::set_column(std::size_t c, const Vector<M, T> & v)
		{
			algorithm{}.set_column_impl(*this, c, v);
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  operator functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t M, std::size_t N, typename T>
		inline Vector<M, T> operator * (const Matrix<M, N, T> & m, const Vector<N, T> & v)
		{
			Vector<M, T> product;

			for (std::size_t row = 0; row < M; row++)
			{
				T dot = T{0};

				for (std::size_t i = 0; i < N; i++)
				{
					dot += m.values[row + i * M] * v.values[i];
				}
				product.values[row] = dot;
			}
			return product;
		}

		////////////////////////////////////////////////////////////////////////
		//
		// function functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t D, typename T>
		Scalar<T> dot(const Plane<D, T> & plane, const Vector<D, T> & point);

		template <std::size_t D, typename T>
		inline Scalar<T> distance(const Plane<D, T> & plane, const Vector<D, T> & point)
		{
			return dot(plane, point) + plane.values[D];
		}
		template <std::size_t D, typename T>
		inline Scalar<T> dot(const Plane<D, T> & plane, const Vector<D, T> & point)
		{
			T s{0};
			for (std::size_t i = 0; i < D; i++)
				s += plane.values[i] * point.values[i];
			return s;
		}

		template <std::size_t D, typename T>
		inline auto dot(const Vector<D, T> & point, const Plane<D, T> & plane) ->
			decltype(dot(plane, point))
		{
			return dot(plane, point);
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  construction functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t D, typename T>
		inline Plane<D, T> make_plane(const Vector<D, T> & point, const Vector<D, T> & normal)
		{
			return utl::unpack(utl::append(normal.values, dot(point, normal)), utl::constructor<Plane<D, T>>{});
		}
	}
}

#endif /* CORE_MATHS_ALGORITHM_HPP */
