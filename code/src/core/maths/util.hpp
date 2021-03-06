#pragma once

#include <utility/type_traits.hpp>

#include <cstdint>

namespace core
{
	namespace maths
	{
		template <typename T>
		struct constant
		{
			static const constexpr T pi = T(3.141592653589793238462643383279502884);
		};

		using constantd = constant<double>;
		using constantf = constant<float>;


		template <typename T>
		class degree;
		template <typename T>
		class radian;

		template <typename T>
		class degree
		{
		public:
			using value_type = T;

		private:
			value_type value;

		public:
			degree() = default;
			explicit degree(const value_type value) : value(value) {}
			template <typename U,
			          typename = mpl::enable_if_t<mpl::fits_in<U, T>::value &&
			                                      mpl::is_different<U, T>::value>>
			degree(const degree<U> & degree) : value(degree.get()) {}
			template <typename U,
			          typename = mpl::enable_if_t<mpl::fits_in<U, T>::value>>
			degree(const radian<U> & radian) : value(value_type{radian.get()} / constant<value_type>::pi * value_type{180}) {}

		public:
			value_type get() const { return this->value; }

		public:
			friend radian<value_type> make_radian(const degree<value_type> & degree)
			{
				return radian<value_type>{value_type(degree.value / 180. *  constantd::pi)};
			}
		};

		template <typename T>
		degree<T> make_degree(const T value)
		{
			return degree<T>{value};
		}
		template <typename T, typename U>
		degree<T> make_degree(const degree<U> & degree)
		{
			return make_degree(T(degree.get()));
		}

		using degreef = degree<float>;
		using degreed = degree<double>;

		template <typename T>
		class radian
		{
		public:
			using value_type = T;

		private:
			value_type value;

		public:
			radian() = default;
			explicit radian(const value_type value) : value(value) {}
			template <typename U,
			          typename = mpl::enable_if_t<mpl::fits_in<U, T>::value &&
			                                      mpl::is_different<U, T>::value>>
			radian(const radian<U> & radian) : value(radian.get()) {}
			template <typename U,
			          typename = mpl::enable_if_t<mpl::fits_in<U, T>::value>>
			radian(const degree<U> & degree) : value(value_type{degree.get()} / value_type{180} *  constant<value_type>::pi) {}

		public:
			value_type get() const { return this->value; }

		public:
			friend degree<value_type> make_degree(const radian<value_type> & radian)
			{
				return degree<value_type>{value_type(radian.value / constantd::pi * 180.)};
			}
		};

		template <typename T>
		radian<T> make_radian(const T value)
		{
			return radian<T>{value};
		}
		template <typename T, typename U>
		radian<T> make_radian(const radian<U> & radian)
		{
			return make_radian(T(radian.get()));
		}

		using radianf = radian<float>;
		using radiand = radian<double>;


		inline constexpr int32_t interpolate_and_scale(int32_t min, int32_t max, int32_t x)
		{
			const uint32_t q = (2 * ((uint32_t(1) << 31) - 1)) / (uint32_t(max) - min);
			const uint32_t r = (2 * ((uint32_t(1) << 31) - 1)) % (uint32_t(max) - min);
			return (uint32_t(x) - min) * q - int32_t((uint32_t(1) << 31) - 1) + r * (uint32_t(x) - min) / (uint32_t(max) - min);
		}
	}
}
