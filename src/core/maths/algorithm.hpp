
#ifndef CORE_MATHS_ALGORITHM_HPP
#define CORE_MATHS_ALGORITHM_HPP

#include <config.h>

namespace core
{
	namespace maths
	{
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
	}
}

#endif /* CORE_MATHS_ALGORITHM_HPP */
