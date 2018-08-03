
#ifndef UTILITY_LOOKUP_TABLE_HPP
#define UTILITY_LOOKUP_TABLE_HPP

#include "concepts.hpp"
#include "type_traits.hpp"

#include <array>
#include <tuple>

namespace utility
{
	template <typename Key, typename ...Values>
	class lookup_table
	{
	private:
		using this_type = lookup_table<Key, Values...>;
		enum { capacity = sizeof...(Values) };

	private:
		std::array<Key, capacity> keys;
		std::tuple<Values...> values;

	public:
		template <typename ...Pairs,
		          REQUIRES((sizeof...(Pairs) == capacity)),
		          REQUIRES((sizeof...(Pairs) != 1 || !mpl::is_same<this_type, mpl::decay_t<Pairs>...>::value ))>
		constexpr explicit lookup_table(Pairs && ...pairs)
			: keys{{pairs.first...}}
			, values(pairs.second...)
		{}

	public:
		constexpr bool contains(const Key & key) const
		{
			return find(key) != std::size_t(-1);
		}

		constexpr std::size_t size() const { return capacity; }

		constexpr std::size_t find(const Key & key) const
		{
			return find_impl(key, mpl::index_constant<0>{});
		}

		template <std::size_t I,
		          REQUIRES((I < capacity))>
		constexpr mpl::type_at<I, Values...> & get_value() { return std::get<I>(values); }
		template <std::size_t I,
		          REQUIRES((I < capacity))>
		constexpr const mpl::type_at<I, Values...> & get_value() const { return std::get<I>(values); }
	private:
		constexpr std::size_t find_impl(const Key & key, mpl::index_constant<capacity>) const
		{
			return std::size_t(-1);
		}
		template <std::size_t I>
		constexpr std::size_t find_impl(const Key & key, mpl::index_constant<I>) const
		{
			return std::get<I>(keys) == key ? I : find_impl(key, mpl::index_constant<I + 1>{});
		}
	};

	template <typename Pair, typename ...Pairs>
	constexpr auto make_lookup_table(Pair && pair, Pairs && ...pairs)
	{
		return lookup_table<typename Pair::first_type, typename Pair::second_type, typename Pairs::second_type...>(std::forward<Pair>(pair), std::forward<Pairs>(pairs)...);
	}
}

#endif /* UTILITY_LOOKUP_TABLE_HPP */
