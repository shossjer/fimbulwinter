
#ifndef CORE_MATHS_VECTOR_HPP
#define CORE_MATHS_VECTOR_HPP

#include <config.h>

#include "Scalar.hpp"

#include <utility/type_traits.hpp>

#include <algorithm>
#include <cmath>

namespace core
{
	namespace maths
	{
		namespace detail
		{
			template <std::size_t N, typename T, typename Derived>
			class Vector
			{
			public:
				using array_type = T[N];
				using value_type = T;

				static constexpr std::size_t capacity = N;
			private:
				using derived_type = Derived;

			protected:
				std::array<value_type, capacity> values;

			public:
				Vector() = default;
				template <typename ...Ps,
				          typename = mpl::enable_if_t<sizeof...(Ps) == capacity>>
				Vector(Ps && ...ps) :
					values{value_type{std::forward<Ps>(ps)}...}
				{
				}

			public:
				derived_type operator + (const derived_type & v) const
				{
					derived_type res;
					for (std::size_t i = 0; i < capacity; i++)
						res.values[i] = this->values[i] + v.values[i];
					return res;
				}
				derived_type & operator += (const derived_type & v)
				{
					return static_cast<Derived &>(*this = *this + v);
				}
				derived_type operator - (const derived_type & v) const
				{
					derived_type res;
					for (std::size_t i = 0; i < capacity; i++)
						res.values[i] = this->values[i] - v.values[i];
					return res;
				}
				derived_type & operator -= (const derived_type & v)
				{
					return static_cast<Derived &>(*this = *this - v);
				}
				derived_type operator * (const Scalar<value_type> s) const
				{
					derived_type res;
					for (std::size_t i = 0; i < capacity; i++)
						res.values[i] = this->values[i] * s.get();
					return res;
				}
				derived_type & operator *= (const Scalar<value_type> s)
				{
					return static_cast<Derived &>(*this = *this * s);
				}
				derived_type operator / (const Scalar<value_type> s) const
				{
					return *this * (value_type{1} / s.get());
				}
				derived_type & operator /= (const Scalar<value_type> s)
				{
					return static_cast<Derived &>(*this = *this / s);
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
				          typename = mpl::enable_if_t<sizeof...(Ps) == capacity>>
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
			};
		}

		template <std::size_t N, typename T>
		class Vector : public detail::Vector<N, T, Vector<N, T>>
		{
		private:
			using base_type = detail::Vector<N, T, Vector<N, T>>;

		public:
			using array_type = typename base_type::array_type;
			using this_type = Vector<N, T>;
			using value_type = typename base_type::value_type;

		public:
			using base_type::Vector;

		public:
			friend this_type operator * (const Scalar<value_type> & s1, const this_type & v2)
			{
				return v2 * s1;
			}

			friend Scalar<value_type> dot(const this_type & v1, const this_type & v2)
			{
				value_type sum = value_type{0};
				for (std::size_t i = 0; i < this_type::capacity; i++)
					sum += v1.values[i] * v2.values[i];
				return sum;
			}
			friend this_type lerp(const this_type & v1, const this_type & v2,
			                      const Scalar<value_type> s)
			{
				return v1 * s.get() + v2 * (value_type{1} - s.get());
			}
		};
		template <typename T>
		class Vector<3, T> : public detail::Vector<3, T, Vector<3, T>>
		{
		private:
			using base_type = detail::Vector<3, T, Vector<3, T>>;

		public:
			using array_type = typename base_type::array_type;
			using this_type = Vector<3, T>;
			using value_type = typename base_type::value_type;

		public:
			using base_type::Vector;

		public:
			friend this_type operator * (const Scalar<value_type> & s1, const this_type & v2)
			{
				return v2 * s1;
			}

			friend this_type cross(const this_type & v1, const this_type & v2)
			{
				return this_type{v1.values[1] * v2.values[2] - v1.values[2] * v2.values[1],
				                 v1.values[2] * v2.values[0] - v1.values[0] * v2.values[2],
				                 v1.values[0] * v2.values[1] - v1.values[1] * v2.values[0]};
			}
			friend Scalar<value_type> dot(const this_type & v1, const this_type & v2)
			{
				value_type sum = value_type{0};
				for (std::size_t i = 0; i < this_type::capacity; i++)
					sum += v1.values[i] * v2.values[i];
				return sum;
			}
			friend this_type lerp(const this_type & v1, const this_type & v2,
			                      const Scalar<value_type> s)
			{
				return v1 * s.get() + v2 * (value_type{1} - s.get());
			}
		};

		using Vector2f = Vector<2, float>;
		using Vector2d = Vector<2, double>;
		using Vector3f = Vector<3, float>;
		using Vector3d = Vector<3, double>;
		using Vector4f = Vector<4, float>;
		using Vector4d = Vector<4, double>;
	}
}

#endif /* CORE_MATHS_VECTOR_HPP */
