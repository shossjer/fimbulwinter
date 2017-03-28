
#ifndef CORE_CONTAINER_COLLECTION_HPP
#define CORE_CONTAINER_COLLECTION_HPP

#include <core/debug.hpp>

#include <utility/type_traits.hpp>

#include <array>
#include <numeric>
#include <tuple>
#include <utility>

// preprocessor hack that might be useful elsewhere also
#define EXPAND_1_TIMES(macro, n) macro((n))
#define EXPAND_2_TIMES(macro, n) EXPAND_1_TIMES(macro, (n)); EXPAND_1_TIMES(macro, (n) + 1)
#define EXPAND_4_TIMES(macro, n) EXPAND_2_TIMES(macro, (n)); EXPAND_2_TIMES(macro, (n) + 2)
#define EXPAND_8_TIMES(macro, n) EXPAND_4_TIMES(macro, (n)); EXPAND_4_TIMES(macro, (n) + 4)
#define EXPAND_16_TIMES(macro, n) EXPAND_8_TIMES(macro, (n)); EXPAND_8_TIMES(macro, (n) + 8)
#define EXPAND_32_TIMES(macro, n) EXPAND_16_TIMES(macro, (n)); EXPAND_16_TIMES(macro, (n) + 16)
#define EXPAND_64_TIMES(macro, n) EXPAND_32_TIMES(macro, (n)); EXPAND_32_TIMES(macro, (n) + 32)
#define EXPAND_128_TIMES(macro, n) EXPAND_64_TIMES(macro, (n)); EXPAND_64_TIMES(macro, (n) + 64)
#define EXPAND_256_TIMES(macro, n) EXPAND_128_TIMES(macro, (n)); EXPAND_128_TIMES(macro, (n) + 128)

namespace core
{
	namespace container
	{
		/**
		 * \tparam Key     The identification/lookup key.
		 * \tparam Maximum Should be about twice as many as is needed.
		 * \tparam Arrays  A std::array for each type (with count) to store.
		 */
		template <typename Key, std::size_t Maximum, typename ...Arrays>
		class Collection;
		template <typename K, std::size_t M, typename ...Cs, std::size_t ...Ns>
		class Collection<K, M, std::array<Cs, Ns>...>
		{
		private:
			using bucket_t = uint32_t;
			using uint24_t = uint32_t;

		public:
			template <typename C, std::size_t N>
			struct array_t
			{
				static constexpr std::size_t capacity = N;

				std::size_t size;
				mpl::aligned_storage_t<sizeof(C), alignof(C)> components[N];
				bucket_t buckets[N];

				array_t() :
					size(0)
				{
				}

				C * begin()
				{
					return reinterpret_cast<C *>(&components) + 0;
				}
				const C * begin() const
				{
					return reinterpret_cast<const C *>(&components) + 0;
				}
				C * end()
				{
					return reinterpret_cast<C *>(&components) + size;
				}
				const C * end() const
				{
					return reinterpret_cast<const C *>(&components) + size;
				}

				C & get(const std::size_t index)
				{
					return reinterpret_cast<C &>(components[index]);
				}
				const C & get(const std::size_t index) const
				{
					return reinterpret_cast<const C &>(components[index]);
				}

				template <typename ...Ps>
				void construct(const std::size_t index, Ps && ...ps)
				{
					new (components + index) C(std::forward<Ps>(ps)...);
				}
				void destruct(const std::size_t index)
				{
					this->get(index).~C();
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
			K keys[M];

		public:
			bool contains(K key)
			{
				return try_find(key) != bucket_t(-1);
			}
			template <typename C>
			auto get() ->
				decltype(std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays))
			{
				return std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays);
			}
			template <typename C>
			auto get() const ->
				decltype(std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays))
			{
				return std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays);
			}

			template <typename C>
			K get_key(const C & component) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto & array = std::get<type>(arrays);
				debug_assert(&component >= array.begin());
				debug_assert(&component < array.end());

				return keys[array.buckets[std::distance(array.begin(), &component)]];
			}

			template <typename D>
			void add(K key, D && data)
			{
				using Components = mpl::type_filter<std::is_constructible,
				                                    mpl::type_list<Cs...>,
				                                    D &&>;
				static_assert(Components::size == 1, "Exactly one type needs to be constructible with the argument.");

				emplace<mpl::type_head<Components>>(key, std::forward<D>(data));
			}
			template <typename Component, typename ...Ps>
			Component & emplace(K key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, mpl::type_list<Cs...>>::value;

				const auto bucket = place(key);
				auto & array = std::get<type>(arrays);
				debug_assert(array.size < array.capacity);
				const auto index = array.size;

				slots[bucket].set(type, index);
				keys[bucket] = key;
				array.construct(index, std::forward<Ps>(ps)...);
				array.buckets[index] = bucket;
				array.size++;

				return array.get(index);
			}
			void remove(K key)
			{
				const auto bucket = find(key);
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
			void update(K key, D && data)
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					set_impl(mpl::index_constant<(std::is_assignable<mpl::type_at_or<(n), \
					                                                                 mpl::type_list<Cs...>>, \
					                                                 D>::value ? \
					                              (n) : \
					                              std::size_t(-1))>{}, \
					         index, std::forward<D>(data)); \
					break

					EXPAND_256_TIMES(CASE, 0);
#undef CASE
				default:
					debug_unreachable();
				}
			}

			template <typename F>
			auto call(K key, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 index, std::forward<F>(func))

					EXPAND_256_TIMES(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 index, std::forward<F>(func));
				}
			}

			template <typename F>
			auto try_call(K key, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				const auto bucket = try_find(key);

				if (bucket == bucket_t(-1)) return func();

				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 index, std::forward<F>(func))

					EXPAND_256_TIMES(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
						index, std::forward<F>(func));
				}
			}
		private:
			// not great
			bucket_t hash(K key) const
			{
				return (std::size_t{key} * std::size_t{key}) % M;
			}
			/**
			 * Find an empty bucket where the key can be placed.
			 */
			bucket_t place(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (!slots[bucket].empty()) // ... this bucket is not empty!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t find(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t try_find(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0;
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					if (count++ >= 4)
						return bucket_t(-1);
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
				// keys[bucket] = ??? // not needed
				if (index < last)
				{
					slots[array.buckets[last]].set_index(index);
					array.get(index) = std::move(array.get(last));
					array.buckets[index] = array.buckets[last];
				}
				array.destruct(last);
				array.size--;
			}
			template <typename D>
			void set_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index, D && data)
			{
				debug_unreachable();
			}
			template <std::size_t type, typename D>
			void set_impl(mpl::index_constant<type>, uint24_t index, D && data)
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				array.get(index) = std::forward<D>(data);
			}

			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				debug_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return func(*reinterpret_cast<mpl::type_head<mpl::type_list<Cs...>> *>(0));
			}
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, uint24_t index, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				return func(array.get(index));
			}
		};

		/**
		 * \tparam Key     The identification/lookup key.
		 * \tparam Maximum Should be about twice as many as is needed.
		 * \tparam Arrays  A std::array for each type (with count) to store.

		 * The unordered collection has the following properties:
		 * - does not move around the components, and
		 * - the components cannot be iterated
		 */
		template <typename Key, std::size_t Maximum, typename ...Arrays>
		class UnorderedCollection;
		template <typename K, std::size_t M, typename ...Cs, std::size_t ...Ns>
		class UnorderedCollection<K, M, std::array<Cs, Ns>...>
		{
		private:
			using bucket_t = uint32_t;
			using uint24_t = uint32_t;

		public:
			template <typename C, std::size_t N>
			struct array_t
			{
				static constexpr std::size_t capacity = N;

				std::size_t size;
				mpl::aligned_storage_t<sizeof(C), alignof(C)> components[N];
				uint24_t free_indices[N];

				array_t() :
					size(0)
				{
					std::iota(free_indices + 0, free_indices + N, 0);
				}

				C & get(const std::size_t index)
				{
					return reinterpret_cast<C &>(components[index]);
				}
				const C & get(const std::size_t index) const
				{
					return reinterpret_cast<const C &>(components[index]);
				}

				template <typename ...Ps>
				void construct(const std::size_t index, Ps && ...ps)
				{
					new (components + index) C(std::forward<Ps>(ps)...);
				}
				void destruct(const std::size_t index)
				{
					this->get(index).~C();
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
			K keys[M];

		public:
			bool contains(K key)
			{
				return try_find(key) != bucket_t(-1);
			}
			template <typename C>
			C & get(K key)
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				debug_assert(slots[bucket].get_type() == type);
				const auto index = slots[bucket].get_index();

				return std::get<type>(arrays).get(index);
			}
			template <typename C>
			const C & get(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				debug_assert(slots[bucket].get_type() == type);
				const auto index = slots[bucket].get_index();

				return std::get<type>(arrays).get(index);
			}

			template <typename D>
			void add(K key, D && data)
			{
				using Components = mpl::type_filter<std::is_constructible,
				                                    mpl::type_list<Cs...>,
				                                    D &&>;
				static_assert(Components::size == 1, "Exactly one type needs to be constructible with the argument.");

				emplace<mpl::type_head<Components>>(key, std::forward<D>(data));
			}
			template <typename Component, typename ...Ps>
			Component & emplace(K key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, mpl::type_list<Cs...>>::value;

				const auto bucket = place(key);
				auto & array = std::get<type>(arrays);
				debug_assert(array.size < array.capacity);
				const auto index = array.free_indices[array.size];

				slots[bucket].set(type, index);
				keys[bucket] = key;
				array.construct(index, std::forward<Ps>(ps)...);
				array.size++;

				return array.get(index);
			}
			void remove(K key)
			{
				const auto bucket = find(key);
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
			void update(K key, D && data)
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					set_impl(mpl::index_constant<(std::is_assignable<mpl::type_at_or<(n), \
					                                                                 mpl::type_list<Cs...>>, \
					                                                 D>::value ? \
					                              (n) : \
					                              std::size_t(-1))>{}, \
					         index, std::forward<D>(data)); \
					break

					EXPAND_256_TIMES(CASE, 0);
#undef CASE
				default:
					debug_unreachable();
				}
			}

			template <typename F>
			auto call(K key, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 index, std::forward<F>(func))

					EXPAND_256_TIMES(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 index, std::forward<F>(func));
				}
			}
		private:
			// not great
			bucket_t hash(K key) const
			{
				return (std::size_t{key} * std::size_t{key}) % M;
			}
			/**
			 * Find an empty bucket where the key can be placed.
			 */
			bucket_t place(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (!slots[bucket].empty()) // ... this bucket is not empty!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t find(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t try_find(K key)
			{
				auto bucket = hash(key);
				std::size_t count = 0;
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					if (count++ >= 4)
						return bucket_t(-1);
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

				slots[bucket].clear();
				// keys[bucket] = ??? // not needed
				array.destruct(index);
				array.free_indices[array.size - 1] = index;
				array.size--;
			}
			template <typename D>
			void set_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index, D && data)
			{
				debug_unreachable();
			}
			template <std::size_t type, typename D>
			void set_impl(mpl::index_constant<type>, uint24_t index, D && data)
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				array.get(index) = std::forward<D>(data);
			}

			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				debug_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return func(*reinterpret_cast<mpl::type_head<mpl::type_list<Cs...>> *>(0));
			}
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, uint24_t index, F && func) ->
				decltype(func(std::declval<mpl::type_head<mpl::type_list<Cs...>> &>()))
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				return func(array.get(index));
			}
		};
	}
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

#endif /* CORE_CONTAINER_COLLECTION_HPP */
