
#ifndef CORE_MATHS_UTIL_HPP
#define CORE_MATHS_UTIL_HPP

namespace core
{
	namespace maths
	{
		template <typename T, typename U>
		struct fits_in;

		template <>
		struct fits_in<float, float>
		{
			using type = void;
		};
		template <>
		struct fits_in<float, double>
		{
			using type = void;
		};
		template <>
		struct fits_in<double, double>
		{
			using type = void;
		};


		template <typename T>
		struct constant
		{
			static const constexpr T pi = 3.141592653589793238462643383279502884;
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
		private:
			T value;

		public:
			degree() = default;
			degree(const degree<T> & degree) = default;
			explicit degree(const T value) : value(value) {}
			template <typename U, typename = typename fits_in<U, T>::type>
			degree(const degree<U> & degree) : value(degree.get()) {}
			template <typename U, typename = typename fits_in<U, T>::type>
			degree(const radian<U> & radian) : value(T(radian.get() / constantd::pi * 180.)) {}

			degree<T> & operator = (const degree<T> & degree) = default;

		public:
			T get() const { return this->value; }

		public:
			friend radian<T> make_radian(const degree<T> & degree)
			{
				return radian<T>{T(degree.value / 180. *  constantd::pi)};
			}
		};

		template <typename T>
		degree<T> make_degree(const T value)
		{
			return degree<T>{value};
		}

		using degreef = degree<float>;
		using degreed = degree<double>;

		template <typename T>
		class radian
		{
		private:
			T value;

		public:
			radian() = default;
			radian(const radian<T> & radian) = default;
			explicit radian(const T value) : value(value) {}
			template <typename U, typename = typename fits_in<U, T>::type>
			radian(const radian<U> & radian) : value(radian.get()) {}
			template <typename U, typename = typename fits_in<U, T>::type>
			radian(const degree<U> & degree) : value(T(degree.get() / 180. *  constantd::pi)) {}

			radian<T> & operator = (const radian<T> & radian) = default;

		public:
			T get() const { return this->value; }

		public:
			friend degree<T> make_degree(const radian<T> & radian)
			{
				return degree<T>{T(radian.value / constantd::pi * 180.)};
			}
		};

		template <typename T>
		radian<T> make_radian(const T value)
		{
			return radian<T>{value};
		}

		using radianf = radian<float>;
		using radiand = radian<double>;
	}
}

#endif /* CORE_MATHS_UTIL_HPP */
