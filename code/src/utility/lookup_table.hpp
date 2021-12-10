
#ifndef UTILITY_LOOKUP_TABLE_HPP
#define UTILITY_LOOKUP_TABLE_HPP

#include "concepts.hpp"
#include "type_traits.hpp"

#include <array>
#include <tuple>

namespace utility
{
	namespace detail
	{
		template <typename T>
		struct cxp_value
		{
			T && t;
		};

		template <typename T>
		constexpr cxp_value<T> cxp(T && t)
		{
			return cxp_value<T>{t};
		}

		namespace detail
		{
			template <typename T1, typename T2>
			constexpr auto equals(cxp_value<T1> && x, cxp_value<T2> && y, int)
				-> decltype(cxp_equals(std::forward<T1>(x.t), std::forward<T2>(y.t)))
			{
				return cxp_equals(std::forward<T1>(x.t), std::forward<T2>(y.t));
			}

			template <typename T1, typename T2>
			constexpr auto equals(cxp_value<T1> && x, cxp_value<T2> && y, ...)
				-> decltype(std::forward<T1>(x.t) == std::forward<T2>(y.t))
			{
				return std::forward<T1>(x.t) == std::forward<T2>(y.t);
			}
		}

		template <typename T1, typename T2>
		constexpr auto operator == (cxp_value<T1> && x, cxp_value<T2> && y)
			-> decltype(detail::equals(static_cast<cxp_value<T1> &&>(x), static_cast<cxp_value<T2> &&>(y), 0))
		{
			return detail::equals(static_cast<cxp_value<T1> &&>(x), static_cast<cxp_value<T2> &&>(y), 0);
		}

		template <typename Key, std::size_t N>
		struct lookup_table_keys
		{
			std::array<Key, N> keys;

			template <typename ...Ps>
			constexpr lookup_table_keys(Ps && ...ps)
				: keys{{std::forward<Ps>(ps)...}}
			{}

			constexpr bool contains(const Key & key) const
			{
				return find(key) != std::size_t(-1);
			}

			constexpr std::size_t find(const Key & key) const
			{
				return find_impl(key, mpl::index_constant<0>{});
			}

			template <std::size_t I,
			          REQUIRES((I < N))>
			constexpr const Key & get_key() const { return std::get<I>(keys); }

			constexpr const Key & get_key(std::ptrdiff_t index) const { return keys[index]; }
		private:
			constexpr std::size_t find_impl(const Key & /*key*/, mpl::index_constant<N>) const
			{
				return std::size_t(-1);
			}
			template <std::size_t I>
			constexpr std::size_t find_impl(const Key & key, mpl::index_constant<I>) const
			{
				return cxp(std::get<I>(keys)) == cxp(key) ? I : find_impl(key, mpl::index_constant<I + 1>{});
			}
		};

		struct invalid_type
		{
			template <typename T>
			operator T & () const
			{
				intrinsic_unreachable();
			}

			template <typename T>
			operator T && () const
			{
				intrinsic_unreachable();
			}
		};

		template <typename Key>
		struct lookup_table_value_empty
		{
			enum { all_values_same_type = true };

			template <typename Value>
			constexpr std::ptrdiff_t find_value(const Value & value) const
			{
				return static_cast<void>(value), std::ptrdiff_t(-1);
			}

			template <std::size_t I,
			          REQUIRES((I < 0))> // note never true
			constexpr invalid_type get_value() const { return invalid_type{}; }

			invalid_type get_value(std::ptrdiff_t index) const
			{
				return static_cast<void>(index), invalid_type{};
			}

			constexpr bool contains(const Key & key) const
			{
				return static_cast<void>(key), false;
			}

			constexpr std::size_t find(const Key & key) const
			{
				return static_cast<void>(key), std::size_t(-1);
			}

			template <std::size_t I,
			          REQUIRES((I < 0))> // note never true
			constexpr const Key & get_key() const { return invalid_type{}; }

			const Key & get_key(std::ptrdiff_t index) const
			{
				return static_cast<void>(index), invalid_type{};
			}
		};

		template <typename Key, typename Value, std::size_t N>
		struct lookup_table_value_array : lookup_table_keys<Key, N>
		{
			using base_type = lookup_table_keys<Key, N>;

			enum { all_values_same_type = true };

			std::array<Value, N> values;

			template <typename ...Pairs>
			constexpr explicit lookup_table_value_array(Pairs && ...pairs)
				: base_type(pairs.first...)
				, values{{pairs.second...}}
			{}

			constexpr std::ptrdiff_t find_value(const Value & value) const
			{
				return find_value_impl(value, mpl::index_constant<0>{});
			}

			template <std::size_t I,
			          REQUIRES((I < N))>
			constexpr const Value & get_value() const { return std::get<I>(values); }

			constexpr const Value & get_value(std::ptrdiff_t index) const { return values[index]; }
		private:
			constexpr std::size_t find_value_impl(const Value & /*value*/, mpl::index_constant<N>) const
			{
				return std::size_t(-1);
			}
			template <std::size_t I>
			constexpr std::size_t find_value_impl(const Value & value, mpl::index_constant<I>) const
			{
				return std::get<I>(values) == value ? I : find_value_impl(value, mpl::index_constant<I + 1>{});
			}
		};

		template <typename Key, typename ...Values>
		struct lookup_table_value_tuple : lookup_table_keys<Key, sizeof...(Values)>
		{
			using base_type = lookup_table_keys<Key, sizeof...(Values)>;

			enum { all_values_same_type = false };

			std::tuple<Values...> values;

			template <typename ...Pairs>
			constexpr explicit lookup_table_value_tuple(Pairs && ...pairs)
				: base_type(pairs.first...)
				, values(pairs.second...)
			{}

			template <std::size_t I,
			          REQUIRES((I < sizeof...(Values)))>
			constexpr const mpl::type_at<I, Values...> & get_value() const { return std::get<I>(values); }
		};

		template <unsigned long long N, typename Key, typename ...Values>
		struct lookup_table_values_impl
			: mpl::conditional<mpl::is_same<Values...>::value,
			                   lookup_table_value_array<Key, mpl::car<Values...>, N>,
			                   lookup_table_value_tuple<Key, Values...>>
		{};

		template <typename Key>
		struct lookup_table_values_impl<0, Key> : mpl::type_is<lookup_table_value_empty<Key>> {};

		template <typename Key, typename ...Values>
		using lookup_table_values = typename lookup_table_values_impl<sizeof...(Values), Key, Values...>::type;
	}

	template <typename Key, typename ...Values>
	class lookup_table : public detail::lookup_table_values<Key, Values...>
	{
	public:
		enum { capacity = sizeof...(Values) };
	private:
		using base_type = detail::lookup_table_values<Key, Values...>;
		using this_type = lookup_table<Key, Values...>;

	public:
		template <typename ...Pairs,
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((sizeof...(Pairs) == sizeof...(Values))),
#else
		          REQUIRES((sizeof...(Pairs) == capacity)),
#endif
		          REQUIRES((sizeof...(Pairs) != 1 || !mpl::is_same<this_type, mpl::decay_t<Pairs>...>::value ))>
		constexpr explicit lookup_table(Pairs && ...pairs)
			: base_type(std::forward<Pairs>(pairs)...)
		{}

	public:
		constexpr std::size_t size() const { return capacity; }
	};

	template <typename Key, typename ...Pairs>
	constexpr auto make_lookup_table(Pairs && ...pairs)
	{
		return lookup_table<Key, typename Pairs::second_type...>(std::forward<Pairs>(pairs)...);
	}

	template <typename Pair, typename ...Pairs>
	constexpr auto make_lookup_table(Pair && pair, Pairs && ...pairs)
	{
		return lookup_table<typename Pair::first_type, typename Pair::second_type, typename Pairs::second_type...>(std::forward<Pair>(pair), std::forward<Pairs>(pairs)...);
	}
}

#endif /* UTILITY_LOOKUP_TABLE_HPP */
