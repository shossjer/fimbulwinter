
#ifndef UTILITY_STORING_HPP
#define UTILITY_STORING_HPP

#include "utility.hpp"

namespace utility
{
	namespace detail
	{
		template <typename T, bool = std::is_trivially_destructible<T>::value>
		struct storing_destructible
		{
			union
			{
				T value;
				char dummy;
			};

			constexpr storing_destructible() = default;
			template <typename U,
			          REQUIRES((std::is_constructible<T, U &&>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, in_place_t>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, null_place_t>::value))>
			constexpr storing_destructible(U && u)
				: value(std::forward<U>(u))
			{}
			constexpr storing_destructible(null_place_t) noexcept
				: dummy()
			{}
			template <typename ...Ps>
			storing_destructible(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
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
		struct storing_destructible<T, false>
		{
			union
			{
				T value;
				char dummy;
			};

			~storing_destructible()
			{}
			storing_destructible(null_place_t) noexcept
				: dummy()
			{}
			template <typename ...Ps>
			storing_destructible(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
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

		template <typename T, bool = std::is_trivial<T>::value>
		struct storing_trivial : storing_destructible<T>
		{
			using value_type = T;

			using storing_destructible<T>::storing_destructible;
		};
		template <typename T>
		struct storing_trivial<T, false>
			: storing_destructible<T>
		{
			using value_type = T;

			storing_trivial() noexcept
				: storing_destructible<T>(null_place)
			{}
			template <typename ...Ps>
			storing_trivial(in_place_t, Ps && ...ps)
				: storing_destructible<T>(in_place, std::forward<Ps>(ps)...)
			{}
		};
	}

	template <typename T>
	using storing = detail::storing_trivial<T>;
}

#endif /* UTILITY_STORING_HPP */
