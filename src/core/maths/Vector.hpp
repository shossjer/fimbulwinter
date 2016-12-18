
#ifndef CORE_MATHS_VECTOR_HPP
#define CORE_MATHS_VECTOR_HPP

#include <config.h>

#include "Scalar.hpp"

#include <utility/algorithm.hpp>
#include <utility/type_traits.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <ostream>

namespace core
{
	namespace maths
	{
		template <std::size_t M, std::size_t N, typename T>
		class Matrix;
		template <std::size_t D, typename T>
		class Plane;
		template <std::size_t N, typename T>
		class Vector;

		struct algorithm;

		template <std::size_t M, std::size_t N, typename T>
		Vector<M, T> operator * (const Matrix<M, N, T> & m, const Vector<N, T> & v);


		template <typename T>
		struct attribute_up;
		template <typename T>
		struct attribute_up<Vector<1, T>>
		{
			using this_type = attribute_up<Vector<1, T>>;

			Vector<1, T> && v;

			attribute_up(Vector<1, T> && v) :
				v(std::move(v))
			{
			}
			attribute_up(const this_type &) = delete;
			this_type & operator = (const this_type &) = delete;
			attribute_up(this_type &&) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename T>
		Vector<2, T> cross(attribute_up<Vector<1, T>> && v1, const Vector<2, T> & v2);
		template <typename T>
		Vector<2, T> cross(const Vector<2, T> & v1, attribute_up<Vector<1, T>> && v2);

		namespace detail
		{
			struct vector_swizzling;
		}
	}
}

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
				Vector() {}
				template <typename ...Ps,
				          typename = mpl::enable_if_t<sizeof...(Ps) == capacity>>
				Vector(Ps && ...ps) :
					values{{value_type{std::forward<Ps>(ps)}...}}
				{
				}

			public:
				derived_type operator + () const
				{
					return *this;
				}
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
				derived_type operator - () const
				{
					derived_type res;
					res.values = utl::flip(this->values);
					return res;
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
					this->values = {{std::forward<Ps>(ps)...}};
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
			friend struct algorithm;
			friend struct detail::vector_swizzling;

		private:
			using base_type = detail::Vector<N, T, Vector<N, T>>;

		public:
			using array_type = typename base_type::array_type;
			using this_type = Vector<N, T>;
			using value_type = typename base_type::value_type;

		public:
			using base_type::base_type;

		public:
			friend this_type operator * (const Scalar<value_type> & s1, const this_type & v2)
			{
				return v2 * s1;
			}
			template <std::size_t M_, std::size_t N_, typename T_>
			friend Vector<M_, T_> operator * (const Matrix<M_, N_, T_> & m, const Vector<N_, T_> & v);

			template <typename T_>
			friend Vector<2, T_> cross(attribute_up<Vector<1, T_>> && v1, const Vector<2, T_> & v2);
			template <typename T_>
			friend Vector<2, T_> cross(const Vector<2, T_> & v1, attribute_up<Vector<1, T_>> && v2);
			template <std::size_t D_, typename T_>
			friend Scalar<T_> dot(const Plane<D_, T_> & plane, const Vector<D_, T_> & point);
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

			template <std::size_t D_, typename T_>
			friend Plane<D_, T_> make_plane(const Vector<D_, T_> & point, const Vector<D_, T_> & normal);
		};
		template <typename T>
		class Vector<2, T> : public detail::Vector<2, T, Vector<2, T>>
		{
			friend struct algorithm;
			friend struct detail::vector_swizzling;

		private:
			using base_type = detail::Vector<2, T, Vector<2, T>>;

		public:
			using array_type = typename base_type::array_type;
			using this_type = Vector<2, T>;
			using value_type = typename base_type::value_type;

		public:
			using base_type::base_type;

		public:
			friend this_type operator * (const Scalar<value_type> & s1, const this_type & v2)
			{
				return v2 * s1;
			}
			template <std::size_t M_, std::size_t N_, typename T_>
			friend Vector<M_, T_> operator * (const Matrix<M_, N_, T_> & m, const Vector<N_, T_> & v);

			friend attribute_up<Vector<1, T>> cross(const this_type & v1, const this_type & v2)
			{
				return Vector<1, T>{v1.values[0] * v2.values[1] - v1.values[1] * v2.values[0]};
			}
			friend value_type cross2(const this_type & v1, const this_type & v2)
			{
				return v1.values[0] * v2.values[1] - v1.values[1] * v2.values[0];
			}
			template <typename T_>
			friend Vector<2, T_> cross(attribute_up<Vector<1, T_>> && v1, const Vector<2, T_> & v2);
			template <typename T_>
			friend Vector<2, T_> cross(const Vector<2, T_> & v1, attribute_up<Vector<1, T_>> && v2);
			template <std::size_t D_, typename T_>
			friend Scalar<T_> dot(const Plane<D_, T_> & plane, const Vector<D_, T_> & point);
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
			friend this_type normalize(const this_type & v)
			{
				return v / std::sqrt(v.values[0] * v.values[0] + v.values[1] * v.values[1]);
			}

			friend this_type get_perp(const this_type & v)
			{
				return this_type{-v.values[1], v.values[0]};
			}

			template <std::size_t D_, typename T_>
			friend Plane<D_, T_> make_plane(const Vector<D_, T_> & point, const Vector<D_, T_> & normal);

			friend std::ostream & operator << (std::ostream & stream, const this_type & v)
			{
				return stream << "(" << v.values[0] << ", " << v.values[1] << ")";
			}

			friend value_type length(const this_type & v)
			{
				return std::sqrt(v.values[0] * v.values[0] + v.values[1] * v.values[1]);
			}

			friend value_type square(const this_type & v)
			{
				return (v.values[0] * v.values[0] + v.values[1] * v.values[1]);
			}
		};
		template <typename T>
		class Vector<3, T> : public detail::Vector<3, T, Vector<3, T>>
		{
			friend struct algorithm;
			friend struct detail::vector_swizzling;

		private:
			using base_type = detail::Vector<3, T, Vector<3, T>>;

		public:
			using array_type = typename base_type::array_type;
			using this_type = Vector<3, T>;
			using value_type = typename base_type::value_type;

		public:
			using base_type::base_type;

		public:
			friend this_type operator * (const Scalar<value_type> & s1, const this_type & v2)
			{
				return v2 * s1;
			}
			template <std::size_t M_, std::size_t N_, typename T_>
			friend Vector<M_, T_> operator * (const Matrix<M_, N_, T_> & m, const Vector<N_, T_> & v);

			friend this_type cross(const this_type & v1, const this_type & v2)
			{
				return this_type{v1.values[1] * v2.values[2] - v1.values[2] * v2.values[1],
				                 v1.values[2] * v2.values[0] - v1.values[0] * v2.values[2],
				                 v1.values[0] * v2.values[1] - v1.values[1] * v2.values[0]};
			}
			template <std::size_t D_, typename T_>
			friend Scalar<T_> dot(const Plane<D_, T_> & plane, const Vector<D_, T_> & point);
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

			template <std::size_t D_, typename T_>
			friend Plane<D_, T_> make_plane(const Vector<D_, T_> & point, const Vector<D_, T_> & normal);

			friend std::ostream & operator << (std::ostream & stream, const this_type & v)
			{
				return stream << "(" << v.values[0] << ", " << v.values[1] << ", " << v.values[2] << ")";
			}

			friend Matrix<4, 4, value_type> make_scale_matrix(const this_type & v)
			{
				return Matrix<4, 4, value_type>{
					  v.values[0], value_type{0}, value_type{0}, value_type{0},
					value_type{0},   v.values[1], value_type{0}, value_type{0},
					value_type{0}, value_type{0},   v.values[2], value_type{0},
					value_type{0}, value_type{0}, value_type{0}, value_type{1}
				};
			}
			friend Matrix<4, 4, value_type> make_translation_matrix(const this_type & v)
			{
				return Matrix<4, 4, value_type>::translation(v.values[0], v.values[1], v.values[2]);
			}

			friend value_type length(const this_type & v)
			{
				return std::sqrt(v.values[0] * v.values[0] + v.values[1] * v.values[1] + v.values[2] * v.values[2]);
			}

			friend value_type square(const this_type & v)
			{
				return (v.values[0] * v.values[0] + v.values[1] * v.values[1] + v.values[2] * v.values[2]);
			}

			value_type volume() const
			{
				return this->values[0] * this->values[1] * this->values[2];
			}
		};

		template <typename T>
		inline Vector<2, T> cross(attribute_up<Vector<1, T>> && v1, const Vector<2, T> & v2)
		{
			return Vector<2, T>{- v1.v.values[0] * v2.values[1],
			                    + v1.v.values[0] * v2.values[0]};
		}
		template <typename T>
		inline Vector<2, T> cross(const Vector<2, T> & v1, attribute_up<Vector<1, T>> && v2)
		{
			return Vector<2, T>{+ v1.values[1] * v2.v.values[0],
			                    - v1.values[0] * v2.v.values[0]};
		}

		namespace detail
		{
			struct vector_swizzling
			{
				template <std::size_t N, typename T>
				inline core::maths::Vector<2, T> to_xy(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1]}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<3, T> to_xyz(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], v.values[2]}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_xyzw(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], v.values[2], v.values[3]}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_x000(const core::maths::Vector<N, T> & v) { return {v.values[0], T{0}, T{0}, T{0}}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_x001(const core::maths::Vector<N, T> & v) { return {v.values[0], T{0}, T{0}, T{1}}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_xy00(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], T{0}, T{0}}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_xy01(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], T{0}, T{1}}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_xyz0(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], v.values[2], T{0}}; }
				template <std::size_t N, typename T>
				inline core::maths::Vector<4, T> to_xyz1(const core::maths::Vector<N, T> & v) { return {v.values[0], v.values[1], v.values[2], T{1}}; }

				template <typename T>
				inline core::maths::Vector<2, T> to_xy(const core::maths::Vector<4, T> & v) { const auto inv_scale = v.values[3] == T{0} ? T{1} : T{1} / v.values[3]; return {v.values[0] * inv_scale, v.values[1] * inv_scale}; }
				template <typename T>
				inline core::maths::Vector<3, T> to_xyz(const core::maths::Vector<4, T> & v) { const auto inv_scale = v.values[3] == T{0} ? T{1} : T{1} / v.values[3]; return {v.values[0] * inv_scale, v.values[1] * inv_scale, v.values[2] * inv_scale}; }
			};
		}

		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 2)>>
		inline Vector<2, T> to_xy(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xy(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 3)>>
		inline Vector<3, T> to_xyz(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xyz(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 4)>>
		inline Vector<4, T> to_xyzw(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xyzw(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 1)>>
		inline Vector<4, T> to_x000(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_x000(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 1)>>
		inline Vector<4, T> to_x001(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_x001(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 2)>>
		inline Vector<4, T> to_xy00(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xy00(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 2)>>
		inline Vector<4, T> to_xy01(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xy01(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 3)>>
		inline Vector<4, T> to_xyz0(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xyz0(v); }
		template <std::size_t N, typename T, typename = mpl::enable_if_t<(N >= 3)>>
		inline Vector<4, T> to_xyz1(const Vector<N, T> & v) { return detail::vector_swizzling{}.to_xyz1(v); }

		using Vector2f = Vector<2, float>;
		using Vector2d = Vector<2, double>;
		using Vector3f = Vector<3, float>;
		using Vector3d = Vector<3, double>;
		using Vector4f = Vector<4, float>;
		using Vector4d = Vector<4, double>;
	}
}

#endif /* CORE_MATHS_VECTOR_HPP */
