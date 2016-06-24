
#ifndef ENGINE_COLLECTION_HPP
#define ENGINE_COLLECTION_HPP

#include <core/debug.hpp>

#include <engine/Entity.hpp>

#include <utility/type_traits.hpp>

#include <array>
#include <tuple>
#include <utility>

// preprocessor hack that might be useful elsewhere also
#define EXPAND_1_TIMES(macro, n) macro(n)
#define EXPAND_2_TIMES(macro, n) EXPAND_1_TIMES(macro, (n)); EXPAND_1_TIMES(macro, (n) + 1)
#define EXPAND_4_TIMES(macro, n) EXPAND_2_TIMES(macro, (n)); EXPAND_2_TIMES(macro, (n) + 2)
#define EXPAND_8_TIMES(macro, n) EXPAND_4_TIMES(macro, (n)); EXPAND_4_TIMES(macro, (n) + 4)
#define EXPAND_16_TIMES(macro, n) EXPAND_8_TIMES(macro, (n)); EXPAND_8_TIMES(macro, (n) + 8)
#define EXPAND_32_TIMES(macro, n) EXPAND_16_TIMES(macro, (n)); EXPAND_16_TIMES(macro, (n) + 16)
#define EXPAND_64_TIMES(macro, n) EXPAND_32_TIMES(macro, (n)); EXPAND_32_TIMES(macro, (n) + 32)
#define EXPAND_128_TIMES(macro, n) EXPAND_64_TIMES(macro, (n)); EXPAND_64_TIMES(macro, (n) + 64)
#define EXPAND_256_TIMES(macro, n) EXPAND_128_TIMES(macro, (n)); EXPAND_128_TIMES(macro, (n) + 128)

namespace engine
{
	template <typename R, typename List, typename = void>
	struct first_assignable : first_assignable<R, mpl::type_tail<List>> {};
	template <typename R, typename List>
	struct first_assignable<R, List,
	                        mpl::void_t<mpl::enable_if_t<std::is_assignable<mpl::type_head<List>, R>::value>>> :
		mpl::type_is<mpl::type_head<List>> {};
	template <typename R, typename ...Ls>
	using first_assignable_t = typename first_assignable<R, mpl::type_list<Ls...>>::type;

	template <std::size_t Maximum, typename ...Arrays>
	class Collection;
	template <std::size_t M, typename ...Cs, std::size_t ...Ns>
	class Collection<M, std::array<Cs, Ns>...>
	{
	private:
		using bucket_t = uint32_t;
		using uint24_t = uint32_t;

	public:
		template <typename C, std::size_t N>
		struct array_t
		{
			std::size_t size;
			C components[N];
			bucket_t buckets[N];

			array_t() :
				size(0)
			{
			}

			const C * begin() const
			{
				return components + 0;
			}
			const C * end() const
			{
				return components + size;
			}
		};
	private:
		struct slot_t
		{
			uint32_t value;

			slot_t() :
				value(0xffffffff)
			{
			}

			bool empty() const
			{
				return value == 0xffffffff;
			}
			uint8_t get_type() const
			{
				return value >> 24;
			}
			uint24_t get_index() const
			{
				return value & 0x00ffffff;
			}
			void clear()
			{
				value = 0xffffffff;
			}
			void set(uint8_t type, uint24_t index)
			{
				debug_assert((index & 0xff000000) == uint32_t{0});
				value = (uint32_t{type} << 24) | index;
			}
			void set_index(uint24_t index)
			{
				debug_assert((index & 0xff000000) == uint32_t{0});
				value = (value & 0xff000000) | index;
			}
		};

	private:
		std::tuple<array_t<Cs, Ns>...> arrays;
		slot_t slots[M];
		engine::Entity entities[M];

	public:
		template <typename C>
		auto get() const ->
			decltype(std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays))
		{
			return std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays);
		}

		template <typename D>
		void add(engine::Entity entity, D && data)
		{
			// Find the first  component type that can  be assigned to
			// by D, the type in  this case is a compile-time constant
			// index to the array containing such components.
			constexpr auto type = mpl::index_of<first_assignable_t<mpl::decay_t<D>, Cs...>,
			                                    mpl::type_list<Cs...>>::value;
			debug_printline(0xffffffff, "adding entity ", entity, " to array ", type);

			const auto bucket = place(entity);
			auto & array = std::get<type>(arrays);
			const auto index = array.size;

			slots[bucket].set(type, index);
			entities[bucket] = entity;
			array.components[index] = std::forward<D>(data);
			array.buckets[index] = bucket;
			array.size++;
		}
		void remove(engine::Entity entity)
		{
			const auto bucket = find(entity);
			const auto index = slots[bucket].get_index();

			switch (slots[bucket].get_type())
			{
#define CASE(n) case (n):	  \
				remove_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, bucket, index); \
				break

				EXPAND_256_TIMES(CASE, 0);
#undef CASE
			default:
				debug_unreachable();
			}
		}
		template <typename D>
		void update(engine::Entity entity, D && data)
		{
			const auto bucket = find(entity);
			const auto index = slots[bucket].get_index();

			switch (slots[bucket].get_type())
			{
#define CASE(n) case (n):	  \
				update_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, index, std::forward<D>(data)); \
				break

				EXPAND_256_TIMES(CASE, 0);
#undef CASE
			default:
				debug_unreachable();
			}
		}
	private:
		bucket_t hash(engine::Entity entity) const
		{
			return (entity * entity) % M;
		}
		/**
		 * Find an empty bucket where the entity can be placed.
		 */
		bucket_t place(engine::Entity entity)
		{
			auto bucket = hash(entity);
			std::size_t count = 0; // debug count that asserts if taken too many steps
			// search again if...
			while (!slots[bucket].empty()) // ... this bucket is not empty!
			{
				debug_assert(count++ < std::size_t{10});
				if (bucket++ >= M - 1)
					bucket -= M;
			}
			return bucket;
		}
		/**
		 * Find the bucket where the entity resides.
		 */
		bucket_t find(engine::Entity entity)
		{
			auto bucket = hash(entity);
			std::size_t count = 0; // debug count that asserts if taken too many steps
			// search again if...
			while (slots[bucket].empty() || // ... this bucket is empty, or
			       entities[bucket] != entity) // ... this is not the right one!
			{
				debug_assert(count++ < std::size_t{10});
				if (bucket++ >= M - 1)
					bucket -= M;
			}
			return bucket;
		}
		void remove_impl(mpl::index_constant<std::size_t(-1)>, bucket_t bucket, uint24_t index)
		{
			debug_unreachable();
		}
		template <std::size_t type>
		void remove_impl(mpl::index_constant<type>, bucket_t bucket, uint24_t index)
		{
			auto & array = std::get<type>(arrays);
			debug_assert(index < array.size);

			const auto last = array.size - 1;

			slots[bucket].clear();
			// entities[bucket] = ???
			if (index < last)
			{
				slots[array.buckets[last]].set_index(index);
				array.components[index] = std::move(array.components[last]);
				array.buckets[index] = array.buckets[last];
			}
			// TODO: free component resources if index == last
			array.size--;
		}
		template <typename D>
		void update_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index, D && data)
		{
			debug_unreachable();
		}
		template <std::size_t type, typename D>
		void update_impl(mpl::index_constant<type>, uint24_t index, D && data)
		{
			auto & array = std::get<type>(arrays);
			debug_assert(index < array.size);

			array.components[index] = std::forward<D>(data);
		}
	};
}

#undef EXPAND_256_TIMES
#undef EXPAND_128_TIMES
#undef EXPAND_64_TIMES
#undef EXPAND_32_TIMES
#undef EXPAND_16_TIMES
#undef EXPAND_8_TIMES
#undef EXPAND_4_TIMES
#undef EXPAND_2_TIMES
#undef EXPAND_1_TIMES

#endif /* ENGINE_COLLECTION_HPP */
