
#ifndef CORE_MATHS_PLANE_HPP
#define CORE_MATHS_PLANE_HPP

#include "Vector.hpp"

#include <utility/type_traits.hpp>

#include <algorithm>
#include <array>

namespace core
{
	namespace maths
	{
		template <typename T>
		class Scalar;
		template <std::size_t N, typename T>
		class Vector;
	}
}

namespace core
{
	namespace maths
	{
		/**
		 * \tparam D dimension.
		 * \tparam T value type.
		 */
		template <std::size_t D, typename T>
		class Plane
		{
		public:
			using array_type = T[D + 1];
			using this_type = Plane<D, T>;
			using value_type = T;

		private:
			std::array<T, D + 1> values;

		public:
			Plane() = default;
			template <typename ...Ps,
			          typename = mpl::enable_if_t<sizeof...(Ps) == (D + 1)>>
			Plane(Ps && ...ps) :
				values{value_type{std::forward<Ps>(ps)}...}
			{
			}

		public:
			array_type & get(array_type & buffer) const
			{
				std::copy(std::begin(this->values), std::end(this->values), std::begin(buffer));
				return buffer;
			}
			array_type & get_aligned(array_type & buffer) const
			{
				return this->get(buffer);
			}

			template <typename ...Ps,
			          typename = mpl::enable_if_t<sizeof...(Ps) == (D + 1)>>
			void set(Ps && ...ps)
			{
				this->values = {std::forward<Ps>(ps)...};
			}
			void set(const array_type & buffer)
			{
				std::copy(std::begin(buffer), std::end(buffer), std::begin(this->values));
			}
			void set_aligned(const array_type & buffer)
			{
				this->set(buffer);
			}

		public:
			Vector<D, T> normal() const
			{
				return utl::unpack_for(this->values,
				                       mpl::make_index_sequence<D>{},
				                       utl::constructor<Vector<D, T>>{});
			}
			T offset() const
			{
				return this->values[D];
			}

			this_type & normalize()
			{
				value_type sq_len{0};
				for (std::size_t i = 0; i < D; i++)
				{
					sq_len += this->values[i] * this->values[i];
				}
				const value_type inv_len = value_type{1} / std::sqrt(sq_len);
				for (std::size_t i = 0; i < D; i++)
				{
					this->values[i] *= inv_len;
				}
				return *this;
			}

		public:
			template <std::size_t D_, typename T_>
			friend Scalar<T_> distance(const Plane<D_, T_> & plane, const Vector<D_, T_> & point);
			template <std::size_t D_, typename T_>
			friend Scalar<T_> dot(const Plane<D_, T_> & plane, const Vector<D_, T_> & point);

		};

		using Plane2f = Plane<2, float>;
		using Plane2d = Plane<2, double>;
		using Plane3f = Plane<3, float>;
		using Plane3d = Plane<3, double>;
	}
}

#endif /* CORE_MATHS_PLANE_HPP */
