#pragma once

#include "utility/concepts.hpp"
#include "utility/type_traits.hpp"

#include <functional>

namespace fun
{
	using namespace std::placeholders;

	namespace detail
	{
		template <typename Lambda>
		struct function_object
			: Lambda
		{
			function_object(Lambda lambda)
				: Lambda(lambda)
			{}

			template <typename D2>
			friend auto operator | (function_object first, function_object<D2> second)
			{
				auto composition = [first, second](auto && ...ps)
				{
					return second(first(std::forward<decltype(ps)>(ps)...));
				};
				return function_object<decltype(composition)>(composition);
			}

			template <typename Value>
			friend auto operator == (function_object left, Value && right)
			{
				auto comparison = [left, &right](auto && ...ps)
				{
					return left(std::forward<decltype(ps)>(ps)...) == std::forward<Value>(right);
				};
				return function_object<decltype(comparison)>(comparison);
			}

			template <typename Value>
			friend auto operator == (Value && left, function_object right)
			{
				auto comparison = [&left, right](auto && ...ps)
				{
					return std::forward<Value>(left) == right(std::forward<decltype(ps)>(ps)...);
				};
				return function_object<decltype(comparison)>(comparison);
			}
		};

		template <typename Lambda>
		auto function(Lambda && lambda)
		{
			return function_object<mpl::remove_cvref_t<Lambda>>(lambda);
		}

		template <typename F, typename ...Ps,
		          REQUIRES((!mpl::disjunction<std::is_placeholder<mpl::remove_cvref_t<Ps>>...>::value))>
		decltype(auto) eval(F && func, Ps && ...ps)
		{
			return std::forward<F>(func)(std::forward<Ps>(ps)...);
		}

		template <typename F, typename ...Ps,
		          REQUIRES((mpl::disjunction<std::is_placeholder<mpl::remove_cvref_t<Ps>>...>::value))>
		auto eval(F && func, Ps && ...ps)
		{
			auto partial = [&](auto && ...qs)
			{
				return std::forward<F>(func)(std::get<std::is_placeholder<mpl::remove_cvref_t<Ps>>::value>(std::forward_as_tuple(std::forward<Ps>(ps), std::forward<decltype(qs)>(qs)...))...);
			};
			return function_object<decltype(partial)>(partial);
		}
	}
}
