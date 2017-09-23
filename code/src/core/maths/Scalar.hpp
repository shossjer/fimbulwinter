
#ifndef CORE_MATHS_SCALAR_HPP
#define CORE_MATHS_SCALAR_HPP

#include <algorithm>
#include <ostream>

namespace core
{
	namespace maths
	{
		template <typename T>
		class Scalar
		{
		public:
			using this_type = Scalar<T>;
			using value_type = T;

		private:
			value_type value;

		public:
			Scalar() = default;
			Scalar(const value_type value) :
				value{value}
			{
			}

		public:
			this_type & operator += (const this_type & s)
			{
				this->value += s.value;
				return *this;
			}
			this_type & operator -= (const this_type & s)
			{
				this->value -= s.value;
				return *this;
			}
			this_type & operator *= (const this_type & s)
			{
				this->value *= s.value;
				return *this;
			}
			this_type & operator /= (const this_type & s)
			{
				this->value /= s.value;
				return *this;
			}

		public:
			value_type get() const
			{
				return this->value;
			}

			void set(const value_type value)
			{
				this->value = value;
			}

		public:
			void inverse()
			{
				this->value = value_type{1} / this->value;
			}

		public:
			friend this_type operator + (const this_type & s1, const this_type & s2)
			{
				return s1.value + s2.value;
			}
			friend this_type operator - (const this_type & s1, const this_type & s2)
			{
				return s1.value - s2.value;
			}
			friend this_type operator * (const this_type & s1, const this_type & s2)
			{
				return s1.value * s2.value;
			}
			friend this_type operator / (const this_type & s1, const this_type & s2)
			{
				return s1.value / s2.value;
			}

			friend bool operator < (const this_type & s1, const this_type & s2)
			{
				return s1.value < s2.value;
			}
			friend bool operator > (const this_type & s1, const this_type & s2)
			{
				return s2.value < s1.value;
			}
			friend bool operator <= (const this_type & s1, const this_type & s2)
			{
				return !(s2.value < s1.value);
			}
			friend bool operator >= (const this_type & s1, const this_type & s2)
			{
				return !(s1.value < s2.value);
			}

			friend this_type abs(const this_type & s)
			{
				return std::abs(s.value);
			}
			friend this_type inverse(const this_type & s)
			{
				return value_type{1} / s.value;
			}

			friend std::ostream & operator << (std::ostream & stream, const this_type & s)
			{
				return stream << s.value;
			}
		};

		template <typename T>
		inline Scalar<T> make_scalar(T value)
		{
			return Scalar<T>{value};
		}

		using Scalarf = Scalar<float>;
		using Scalard = Scalar<double>;
	}
}

#endif /* CORE_MATHS_SCALAR_HPP */
