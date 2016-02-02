
#ifndef CORE_MATHS_SCALAR_HPP
#define CORE_MATHS_SCALAR_HPP

#include <config.h>

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
			this_type operator + (const this_type & s) const
			{
				return this->value + s.value;
			}
			this_type & operator += (const this_type & s)
			{
				this->value += s.value;
				return *this;
			}
			this_type operator - (const this_type & s) const
			{
				return this->value - s.value;
			}
			this_type & operator -= (const this_type & s)
			{
				this->value -= s.value;
				return *this;
			}
			this_type operator * (const this_type & s) const
			{
				return this->value * s.value;
			}
			this_type & operator *= (const this_type & s)
			{
				this->value *= s.value;
				return *this;
			}
			this_type operator / (const this_type & s) const
			{
				return this->value / s.value;
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
			friend this_type inverse(const this_type & s)
			{
				return value_type{1} / s.value;
			}
		};
	}
}

#endif /* CORE_MATHS_SCALAR_HPP */
