#pragma once

#include <iostream>

namespace utility
{
	template <typename Derived, typename T>
	class Arithmetic
	{
	private:
		using this_type = Arithmetic<Derived, T>;

	private:
		T value;

	public:
		Arithmetic() = default;
		explicit constexpr Arithmetic(T value)
			: value(value)
		{}
		constexpr Arithmetic & operator = (T value)
		{
			return *this = Arithmetic(value);
		}

		explicit constexpr operator T () const { return value; }
	private:
		static constexpr this_type & as_this(Derived & x) { return x; }
		static constexpr const this_type & as_this(const Derived & x) { return x; }

	public:
		constexpr T get() const { return value; }

		Derived & operator ++ () { ++value; return *this; }
		Derived operator ++ (int) { auto tmp = *this; value++; return tmp; }
		Derived & operator -- () { --value; return *this; }
		Derived operator -- (int) { auto tmp = *this; value--; return tmp; }

		Derived & operator += (const T & value)
		{
			this->value += value;
			return static_cast<Derived &>(*this);
		}
		Derived & operator += (const Derived & other)
		{
			return *this += as_this(other).value;
		}
		Derived & operator -= (const T & value)
		{
			this->value -= value;
			return static_cast<Derived &>(*this);
		}
		Derived & operator -= (const Derived & other)
		{
			return *this -= as_this(other).value;
		}
		Derived & operator *= (const T & value)
		{
			this->value *= value;
			return static_cast<Derived &>(*this);
		}
		Derived & operator *= (const Derived & other)
		{
			return *this *= as_this(other).value;
		}
		Derived & operator /= (const T & value)
		{
			this->value /= value;
			return static_cast<Derived &>(*this);
		}
		Derived & operator /= (const Derived & other)
		{
			return *this /= as_this(other).value;
		}

		friend constexpr Derived operator + (const Derived & x, const T & value)
		{
			return Derived(as_this(x).value + value);
		}
		friend constexpr Derived operator + (const T & value, const Derived & x)
		{
			return Derived(value + as_this(x).value);
		}
		friend constexpr Derived operator + (const Derived & x, const Derived & y)
		{
			return Derived(as_this(x).value + as_this(y).value);
		}
		friend constexpr Derived operator - (const Derived & x, const T & value)
		{
			return Derived(as_this(x).value - value);
		}
		friend constexpr Derived operator - (const T & value, const Derived & x)
		{
			return Derived(value - as_this(x).value);
		}
		friend constexpr Derived operator - (const Derived & x, const Derived & y)
		{
			return Derived(as_this(x).value - as_this(y).value);
		}
		friend constexpr Derived operator * (const Derived & x, const T & value)
		{
			return Derived(as_this(x).value * value);
		}
		friend constexpr Derived operator * (const T & value, const Derived & x)
		{
			return Derived(value * as_this(x).value);
		}
		friend constexpr Derived operator * (const Derived & x, const Derived & y)
		{
			return Derived(as_this(x).value * as_this(y).value);
		}
		friend constexpr Derived operator / (const Derived & x, const T & value)
		{
			return Derived(as_this(x).value / value);
		}
		friend constexpr Derived operator / (const T & value, const Derived & x)
		{
			return Derived(value / as_this(x).value);
		}
		friend constexpr Derived operator / (const Derived & x, const Derived & y)
		{
			return Derived(as_this(x).value / as_this(y).value);
		}

		friend constexpr bool operator == (const Derived & x, const T & value)
		{
			return as_this(x).value == value;
		}
		friend constexpr bool operator == (const T & value, const Derived & x)
		{
			return value == as_this(x).value;
		}
		friend constexpr bool operator == (const Derived & x, const Derived & y)
		{
			return as_this(x).value == as_this(y).value;
		}
		friend constexpr bool operator != (const Derived & x, const T & value)
		{
			return as_this(x).value != value;
		}
		friend constexpr bool operator != (const T & value, const Derived & x)
		{
			return value != as_this(x).value;
		}
		friend constexpr bool operator != (const Derived & x, const Derived & y)
		{
			return as_this(x).value != as_this(y).value;
		}
		friend constexpr bool operator < (const Derived & x, const T & value)
		{
			return as_this(x).value < value;
		}
		friend constexpr bool operator < (const T & value, const Derived & x)
		{
			return value < as_this(x).value;
		}
		friend constexpr bool operator < (const Derived & x, const Derived & y)
		{
			return as_this(x).value < as_this(y).value;
		}
		friend constexpr bool operator <= (const Derived & x, const T & value)
		{
			return as_this(x).value <= value;
		}
		friend constexpr bool operator <= (const T & value, const Derived & x)
		{
			return value <= as_this(x).value;
		}
		friend constexpr bool operator <= (const Derived & x, const Derived & y)
		{
			return as_this(x).value <= as_this(y).value;
		}
		friend constexpr bool operator > (const Derived & x, const T & value)
		{
			return as_this(x).value > value;
		}
		friend constexpr bool operator > (const T & value, const Derived & x)
		{
			return value > as_this(x).value;
		}
		friend constexpr bool operator > (const Derived & x, const Derived & y)
		{
			return as_this(x).value > as_this(y).value;
		}
		friend constexpr bool operator >= (const Derived & x, const T & value)
		{
			return as_this(x).value >= value;
		}
		friend constexpr bool operator >= (const T & value, const Derived & x)
		{
			return value >= as_this(x).value;
		}
		friend constexpr bool operator >= (const Derived & x, const Derived & y)
		{
			return as_this(x).value >= as_this(y).value;
		}

		friend std::ostream & operator << (std::ostream & os, const Derived & x)
		{
			return os << as_this(x).value;
		}
	};
}
