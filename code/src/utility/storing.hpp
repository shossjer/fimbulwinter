#pragma once

#include "utility/utility.hpp"

namespace utility
{
	namespace detail
	{
		template <typename T, bool = std::is_trivially_destructible<T>::value>
		struct storing_trivially_destructible
		{
			union
			{
				T value;
				char dummy;
			};

			constexpr storing_trivially_destructible() = default;

			template <typename U,
			          REQUIRES((std::is_constructible<T, U &&>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, in_place_t>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, null_place_t>::value))>
			explicit constexpr storing_trivially_destructible(U && u)
				: value(std::forward<U>(u))
			{}

			explicit constexpr storing_trivially_destructible(null_place_t) noexcept
				: dummy()
			{}

			template <typename ...Ps,
			          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
			explicit constexpr storing_trivially_destructible(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
			{}

			template <typename ...Ps,
			          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
			          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
			explicit constexpr storing_trivially_destructible(in_place_t, Ps && ...ps)
				: value{std::forward<Ps>(ps)...}
			{}

			template <typename ...Ps>
			T & construct(Ps && ...ps)
			{
				return construct_at<T>(&value, std::forward<Ps>(ps)...);
			}

			void destruct()
			{
			}
		};

		template <typename T>
		struct storing_trivially_destructible<T, false>
		{
			union
			{
				T value;
				char dummy;
			};

			~storing_trivially_destructible()
			{}

			constexpr storing_trivially_destructible() = default;

			explicit constexpr storing_trivially_destructible(null_place_t) noexcept
				: dummy()
			{}

			template <typename U,
			          REQUIRES((std::is_constructible<T, U &&>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, in_place_t>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, null_place_t>::value))>
			explicit constexpr storing_trivially_destructible(U && u)
				: value(std::forward<U>(u))
			{}

			template <typename ...Ps,
			          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
			explicit constexpr storing_trivially_destructible(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
			{}

			template <typename ...Ps,
			          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
			          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
			explicit constexpr storing_trivially_destructible(in_place_t, Ps && ...ps)
				: value{std::forward<Ps>(ps)...}
			{}

			template <typename ...Ps>
			T & construct(Ps && ...ps)
			{
				return construct_at<T>(&value, std::forward<Ps>(ps)...);
			}

			void destruct()
			{
				value.T::~T();
			}
		};

		template <typename T, bool = std::is_trivially_default_constructible<T>::value>
		struct storing_trivially_default_constructible : storing_trivially_destructible<T>
		{
			using value_type = T;

			using storing_trivially_destructible<T>::storing_trivially_destructible;
		};

		template <typename T>
		struct storing_trivially_default_constructible<T, false /*is trivially default constructible*/>
			: storing_trivially_destructible<T>
		{
			using value_type = T;

			explicit constexpr storing_trivially_default_constructible() noexcept
				: storing_trivially_destructible<T>(null_place)
			{}

			template <typename U,
			          REQUIRES((std::is_constructible<T, U &&>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, in_place_t>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, null_place_t>::value))>
			explicit constexpr storing_trivially_default_constructible(U && u)
				: storing_trivially_destructible<T>(std::forward<U>(u))
			{}

			template <typename ...Ps>
			explicit storing_trivially_default_constructible(in_place_t, Ps && ...ps)
				: storing_trivially_destructible<T>(in_place, std::forward<Ps>(ps)...)
			{}
		};
	}

	template <typename T>
	using storing = detail::storing_trivially_default_constructible<T>;
}
