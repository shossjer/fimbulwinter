
#ifndef UTILITY_FUNCTIONAL_HPP
#define UTILITY_FUNCTIONAL_HPP

namespace utility
{
	template <typename T = void>
	struct equal_to; // TODO: std
	template <>
	struct equal_to<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x == y;
		}
	};
	template <typename T = void>
	struct not_equal_to; // TODO: std
	template <>
	struct not_equal_to<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x != y;
		}
	};
	template <typename T = void>
	struct greater; // TODO: std
	template <>
	struct greater<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x > y;
		}
	};
	template <typename T = void>
	struct less; // TODO: std
	template <>
	struct less<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x < y;
		}
	};
	template <typename T = void>
	struct greater_equal; // TODO: std
	template <>
	struct greater_equal<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x >= y;
		}
	};
	template <typename T = void>
	struct less_equal; // TODO: std
	template <>
	struct less_equal<void>
	{
		template <typename X, typename Y>
		bool operator () (X && x, Y && y)
		{
			return x <= y;
		}
	};
}

#endif /* UTILITY_FUNCTIONAL_HPP */
