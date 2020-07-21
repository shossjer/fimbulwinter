
#ifndef CORE_CONTAINER_COLLECTION_HPP
#define CORE_CONTAINER_COLLECTION_HPP

#include "core/debug.hpp"

#include "utility/annotate.hpp"
#include "utility/bitmanip.hpp"
#include "utility/container/array.hpp"
#include "utility/container/fragmentation.hpp"
#include "utility/container/vector.hpp"
#include "utility/preprocessor/expand.hpp"
#include "utility/ranges.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

#include <array>
#include <numeric>
#include <tuple>
#include <utility>

namespace core
{
	namespace container
	{
		namespace detail
		{
			template <typename F, typename K>
			auto call_impl_func(F && func, K key) ->
				decltype(func(key, utility::monostate{}))
			{
				return func(key, utility::monostate{});
			}
			template <typename F, typename K>
			auto call_impl_func(F && func, K /*key*/) ->
				decltype(func(utility::monostate{}))
			{
				return func(utility::monostate{});
			}

			template <typename F, typename K, typename P>
			auto call_impl_func(F && func, K key, P && p) ->
				decltype(func(key, std::forward<P>(p)))
			{
				return func(key, std::forward<P>(p));
			}
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
			// C4702 - unreachable code
#endif
			template <typename F, typename K, typename P>
			auto call_impl_func(F && func, K /*key*/, P && p) ->
				decltype(func(std::forward<P>(p)))
			{
				return func(std::forward<P>(p));
			}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
		}

		/**
		 * \tparam Key                 The identification/lookup key.
		 * \tparam LookupStorageTraits A storage traits for the lookup table.
		 * \tparam ComponentStorages   A storage for each component.
		 */
		template <typename Key, typename LookupStorageTraits, typename ...ComponentStorages>
		class Collection
		{
#if !(defined(_MSC_VER) && _MSC_VER <= 1926)
			static_assert(mpl::conjunction<mpl::bool_constant<(utility::storage_size<ComponentStorages>::value == 1)>...>::value, "Collection does not support multi-type storages for components");
#endif

		private:
			using component_types = mpl::type_list<typename ComponentStorages::template value_type_at<0>...>;

		private:
			using bucket_t = uint32_t;
			using uint24_t = uint32_t;

		private:
			struct slot_t
			{
				uint32_t value;

				uint8_t get_type() const
				{
					return value >> 24;
				}

				uint24_t get_index() const
				{
					return value & 0x00ffffff;
				}

				void set(uint8_t type, std::size_t index)
				{
					debug_assert(index < 0x01000000, "24 bits are not enough");
					value = (uint32_t{type} << 24) | static_cast<uint32_t>(index);
				}

				void set_index(std::size_t index)
				{
					debug_assert(index < 0x01000000, "24 bits are not enough");
					value = (value & 0xff000000) | static_cast<uint32_t>(index);
				}
			};

			struct relocate_rehash
			{
				template <typename Data>
				bool operator () (Data & new_data, Data & old_data)
				{
					const auto new_size = new_data.capacity();
					new_data.storage().memset_fill(new_data.begin_storage(), new_size, ext::byte{});

					const auto old_size = old_data.capacity();
					for (auto i : ranges::index_sequence(old_size))
					{
						if (old_data.storage().data(old_data.begin_storage())[i].second == Key{})
							continue; // empty

						const auto new_bucket = find_empty_bucket(old_data.storage().data(old_data.begin_storage())[i].second, new_data.storage().data(new_data.begin_storage()).second, new_size);
						if (!debug_verify(new_bucket != bucket_t(-1), "collision when reallocating hash"))
							return false; // todo try with bigger allocation?

						using utility::iter_move;
						new_data.storage().data(new_data.begin_storage())[new_bucket] = iter_move(old_data.storage().data(old_data.begin_storage()) + i);
					}
					return true;
				}
			};

		private:
			utility::array<typename LookupStorageTraits::template storage_type<slot_t, Key>, utility::initialize_zero, utility::reserve_nonempty<utility::reserve_power_of_two>::template type, relocate_rehash> lookup_;
			// todo keys before slots?
			std::tuple<utility::vector<typename utility::storage_traits<ComponentStorages>::template append<Key>>...> arrays_;

			decltype(auto) slots() { return lookup_.data().first; }
			decltype(auto) slots() const { return lookup_.data().first; }

			decltype(auto) keys() { return lookup_.data().second; }
			decltype(auto) keys() const { return lookup_.data().second; }

		public:
			template <typename K>
			annotate_nodiscard
			const Key * find_key(K key) const
			{
				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr; // todo weird

				return keys() + bucket;
			}

			template <typename K>
			annotate_nodiscard
			bool contains(K key) const
			{
				return find(key) != bucket_t(-1);
			}

			template <typename C, typename K>
			annotate_nodiscard
			bool contains(K key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return false;

				return slots()[bucket].get_type() == type;
			}

			template <typename C>
			annotate_nodiscard
			utility::span<C> get()
			{
				auto & array = std::get<mpl::index_of<C, component_types>::value>(arrays_);
				return {array.data().first, array.size()};
			}

			template <typename C>
			annotate_nodiscard
			utility::span<const C> get() const
			{
				const auto & array = std::get<mpl::index_of<C, component_types>::value>(arrays_);
				return {array.data().first, array.size()};
			}

			template <typename C, typename K>
			annotate_nodiscard
			C * try_get(K key)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				if (slots()[bucket].get_type() != type)
					return nullptr;

				const auto index = slots()[bucket].get_index();
				return &std::get<type>(arrays_)[index].first;
			}

			template <typename C, typename K>
			annotate_nodiscard
			const C * try_get(Key key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				if (slots()[bucket].get_type() != type)
					return nullptr;

				const auto index = slots()[bucket].get_index();
				return &std::get<type>(arrays_)[index].first;
			}

			template <typename C>
			annotate_nodiscard
			Key get_key(const C & component) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto & array = std::get<type>(arrays_);
				const auto index = &component - array.data().first;
				if (!debug_assert(index < array.size()))
					return Key{};

				return array[index].second;
			}

			void clear()
			{
				utl::for_each(
					arrays_,
					[this](auto & array)
					{
						// todo add memset to ext
						std::memset(array.data().second, static_cast<int>(ext::byte{}), array.size() * sizeof(Key));
						array.clear();
					});
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * try_emplace(Key key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, component_types>::value;

				debug_assert(!contains(key));

				const auto bucket = try_place(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				auto & array = std::get<type>(arrays_);
				const auto index = array.size();

				if (!array.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(std::forward<Ps>(ps)...), std::forward_as_tuple(key)))
					return nullptr;

				slots()[bucket].set(type, index);
				keys()[bucket] = key;

				return &array[index].first;
			}

			template <typename K>
			void remove(K key)
			{
				const auto bucket = find(key);
				const auto index = slots()[bucket].get_index();

				switch (slots()[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					remove_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, bucket, index); \
					break

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					intrinsic_unreachable();
				}
			}

			template <typename K, typename F>
			auto call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), std::declval<Key>(), std::declval<mpl::car<component_types> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots()[bucket].get_index();

				switch (slots()[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 keys()[bucket], index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 keys()[bucket], index, std::forward<F>(func));
				}
			}

			template <typename K, typename F>
			auto try_call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), std::declval<Key>(), std::declval<mpl::car<component_types> &>()))
			{
				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return detail::call_impl_func(std::forward<F>(func), key);

				const auto index = slots()[bucket].get_index();

				switch (slots()[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 keys()[bucket], index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 keys()[bucket], index, std::forward<F>(func));
				}
			}

		private:
			template <typename K>
			static bucket_t find_bucket(K key, const Key * keys, ext::usize nkeys, std::size_t first_bucket)
			{
				auto bucket = first_bucket;
				int count = 0;
				while (keys[bucket] != key)
				{
					if (count >= 4) // arbitrary
						return bucket_t(-1);

					count++;
					bucket = (first_bucket + count * count) % nkeys;
				}
				return static_cast<bucket_t>(bucket);
			}

			template <typename K>
			static bucket_t find_bucket(K key, const Key * keys, ext::usize nkeys)
			{
				if (!debug_assert(nkeys != 0))
					return bucket_t(-1);

				const auto first_bucket = (std::size_t(key) * std::size_t(key)) % nkeys; // todo

				return find_bucket(key, keys, nkeys, first_bucket);
			}

			template <typename K>
			static bucket_t find_empty_bucket(K key, const Key * keys, ext::usize nkeys)
			{
				if (!debug_assert(nkeys != 0))
					return bucket_t(-1);

				const auto first_bucket = (std::size_t(key) * std::size_t(key)) % nkeys; // todo

				return find_bucket(Key{}, keys, nkeys, first_bucket);
			}

			bucket_t try_place(Key key)
			{
				while (true)
				{
					const auto bucket = find_empty_bucket(key, keys(), lookup_.size());
					if (bucket != bucket_t(-1))
						return bucket;

					if (!debug_verify(lookup_.try_reserve(lookup_.capacity() + 1)))
						return bucket_t(-1);
				}
			}

			template <typename K>
			bucket_t find(K key) const
			{
				return find_bucket(key, keys(), lookup_.size());
			}

			void remove_impl(mpl::index_constant<std::size_t(-1)>, bucket_t /*bucket*/, uint24_t /*index*/)
			{
				intrinsic_unreachable();
			}

			template <std::size_t type>
			void remove_impl(mpl::index_constant<type>, bucket_t bucket, uint24_t index)
			{
				auto & array = std::get<type>(arrays_);
				debug_assert(index < array.size());

				const auto last_index = array.size() - 1;
				const auto last_bucket = find(array[last_index].second);

				slots()[last_bucket].set_index(index);
				keys()[bucket] = Key{};
				array.erase(array.begin() + index);
			}

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
			// C4702 - unreachable code
#endif
			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, Key key, uint24_t /*index*/, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				intrinsic_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return detail::call_impl_func(std::forward<F>(func), key, *reinterpret_cast<mpl::car<component_types> *>(0));
			}
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, Key key, uint24_t index, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				auto & array = std::get<type>(arrays_);
				debug_assert(index < array.size());

				return detail::call_impl_func(std::forward<F>(func), key, array[index].first);
			}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
		};

		/**
		 * \tparam Key     The identification/lookup key.
		 * \tparam Maximum Should be about twice as many as is needed.
		 * \tparam Arrays  A std::array for each type (with count) to store.

		 * The multi collection has the following properties:
		 * - it can hold at most one component of each type for a given key
		 */
		template <typename Key, std::size_t Maximum, typename ...Arrays>
		class MultiCollection;
		template <typename K, std::size_t M, typename ...Cs, std::size_t ...Ns>
		class MultiCollection<K, M, std::array<Cs, Ns>...>
		{
		private:
			using bucket_t = uint32_t;

		public:
			template <typename C, std::size_t N>
			struct array_t
			{
				static constexpr std::size_t capacity = N;

				std::size_t size = 0;
				utility::static_storage<N, C> components_; // todo replace with vector
				bucket_t buckets[N];

				C * begin() { return components_.data(components_.begin()); }
				const C * begin() const { return components_.data(components_.begin()); }
				C * end() { return components_.data(components_.begin()) + size; }
				const C * end() const { return components_.data(components_.begin()) + size; }

				C & get(const std::size_t index) { return begin()[index]; }
				const C & get(const std::size_t index) const { return begin()[index]; }

				template <typename ...Ps>
				void construct(const std::size_t index, Ps && ...ps) { components_.construct_at_(components_.begin() + index, std::forward<Ps>(ps)...); }
				void destruct(const std::size_t index) { components_.destruct_at(components_.begin() + index); }
			};
		private:
			struct slot_t
			{
				static_assert(sizeof...(Cs) <= 64, "the type mask only supports at most 64 types");

				uint64_t type_mask;
				uint16_t indices[sizeof...(Cs)];

				slot_t() :
					type_mask(0)
				{}

				bool empty() const
				{
					return type_mask == uint64_t(0);
				}
				template <size_t type>
				bool empty() const
				{
					return (type_mask & uint64_t(1) << uint64_t(type)) == uint64_t(0);
				}
				int get_first_type() const
				{
					debug_assert(!empty());
					return utility::ntz(type_mask);
				}
				template <size_t type>
				uint16_t get_index() const
				{
					debug_assert(!empty<type>());
					return indices[type];
				}
				template <size_t type>
				void clear()
				{
					type_mask &= ~(uint64_t(1) << uint64_t(type));
				}
				template <size_t type>
				void set(uint16_t index)
				{
					type_mask |= uint64_t(1) << uint64_t(type);
					indices[type] = index;
				}
				template <size_t type>
				void set_index(uint16_t index)
				{
					indices[type] = index;
				}
			};

		private:
			std::tuple<array_t<Cs, Ns>...> arrays;
			slot_t slots[M];
			K keys[M];

		public:
			annotate_nodiscard
			bool contains(K key) const
			{
				return try_find(key) != bucket_t(-1);
			}

			template <typename C>
			annotate_nodiscard
			bool contains(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return false;

				return !slots[bucket].template empty<type>();
			}

			template <typename C>
			annotate_nodiscard
			auto get() ->
				decltype(std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays))
			{
				return std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays);
			}

			template <typename C>
			annotate_nodiscard
			auto get() const ->
				decltype(std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays))
			{
				return std::get<mpl::index_of<C, mpl::type_list<Cs...>>::value>(arrays);
			}

			template <typename C>
			annotate_nodiscard
			C & get(K key)
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				return std::get<type>(arrays).get(index);
			}

			template <typename C>
			annotate_nodiscard
			const C & get(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				return std::get<type>(arrays).get(index);
			}

			template <typename C>
			annotate_nodiscard
			K get_key(const C & component) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto & array = std::get<type>(arrays);
				debug_assert(&component >= array.begin());
				debug_assert(&component < array.end());

				return keys[array.buckets[std::distance(array.begin(), &component)]];
			}

			template <typename Component, typename ...Ps>
			Component & emplace(K key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, mpl::type_list<Cs...>>::value;

				const auto bucket = place(key);
				debug_assert(slots[bucket].template empty<type>());

				auto & array = std::get<type>(arrays);
				debug_assert(array.size < array.capacity);
				const auto index = array.size;

				slots[bucket].template set<type>(static_cast<uint16_t>(index));
				keys[bucket] = key;
				array.construct(index, std::forward<Ps>(ps)...);
				array.buckets[index] = bucket;
				array.size++;

				return array.get(index);
			}

			template <typename C>
			void remove(K key)
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				remove_at_impl<type>(bucket, index);
			}

			void remove(K key)
			{
				remove_impl(find(key), mpl::make_index_sequence<sizeof...(Cs)>{});
			}

			template <typename C>
			void try_remove(K key)
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return;

				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				remove_at_impl<type>(bucket, index);
			}

			template <typename F>
			auto call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				const auto bucket = find(key);

				switch (slots[bucket].get_first_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 key, bucket, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 key, bucket, std::forward<F>(func));
				}
			}

			template <typename C, typename F>
			auto call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				return call_impl(mpl::index_constant<type>{},
				                 key, find(key), std::forward<F>(func));
			}

			template <typename F>
			void call_all(K key, F && func)
			{
				call_all_impl(key, find(key), std::forward<F>(func), mpl::make_index_sequence<sizeof...(Cs)>{});
			}

			template <typename F>
			auto try_call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return detail::call_impl_func(std::forward<F>(func), key);

				switch (slots[bucket].get_first_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 key, bucket, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 key, bucket, std::forward<F>(func));
				}
			}

			template <typename C, typename F>
			auto try_call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return detail::call_impl_func(std::forward<F>(func), key);

				if (slots[bucket].template empty<type>())
					return detail::call_impl_func(std::forward<F>(func), key);

				return call_impl(mpl::index_constant<type>{},
				                 key, bucket, std::forward<F>(func));
			}

			template <typename F>
			void try_call_all(K key, F && func)
			{
				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return detail::call_impl_func(std::forward<F>(func), key);

				call_all_impl(key, bucket, std::forward<F>(func), mpl::make_index_sequence<sizeof...(Cs)>{});
			}
		private:
			// not great
			bucket_t hash(K key) const
			{
				return (std::size_t(key) * std::size_t(key)) % M;
			}

			/**
			 * Find an empty bucket where the key can be placed.
			 */
			bucket_t place(K key)
			{
				const auto maybe_bucket = try_find(key);
				if (maybe_bucket != bucket_t(-1))
					return maybe_bucket;

				auto bucket = hash(key);
				debug_expression(int count = 0); // debug count that asserts if taken too many steps
				// search again if...
				while (!slots[bucket].empty()) // ... this bucket is not empty!
				{
					debug_assert(count++ < 4);
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}

			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t find(K key) const
			{
				auto bucket = hash(key);
				debug_expression(int count = 0); // debug count that asserts if taken too many steps
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					debug_assert(count++ < 4);
					if (bucket++ >= M - 1)
						bucket -= M;
				}
				return bucket;
			}

			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t try_find(K key) const
			{
				auto bucket = hash(key);
				int count = 0;
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

			template <size_t type>
			void remove_at_impl(bucket_t bucket, uint16_t index)
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				const auto last = array.size - 1;

				slots[bucket].template clear<type>();
				// keys[bucket] = ??? // not needed
				if (index < last)
				{
					slots[array.buckets[last]].template set_index<type>(index);
					array.get(index) = std::move(array.get(last));
					array.buckets[index] = array.buckets[last];
				}
				array.destruct(last);
				array.size--;
			}

			void remove_impl(bucket_t /*bucket*/, mpl::index_sequence<>)
			{}
			template <size_t type, size_t ...types>
			void remove_impl(bucket_t bucket, mpl::index_sequence<type, types...>)
			{
				if (!slots[bucket].template empty<type>())
				{
					const auto index = slots[bucket].template get_index<type>();
					debug_assert(index >= 0);

					remove_at_impl<type>(bucket, index);
				}
				remove_impl(bucket, mpl::index_sequence<types...>{});
			}

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
			// C4702 - unreachable code
#endif
			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, K key, bucket_t /*bucket*/, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				intrinsic_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return detail::call_impl_func(std::forward<F>(func), key, *reinterpret_cast<mpl::car<Cs...> *>(0));
			}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, K key, bucket_t bucket, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size);

				return detail::call_impl_func(std::forward<F>(func), key, array.get(index));
			}

			template <typename F>
			void call_all_impl(K /*key*/, bucket_t /*bucket*/, F && /*func*/, mpl::index_sequence<>)
			{}
			template <typename F, size_t type, size_t ...types>
			void call_all_impl(K key, bucket_t bucket, F && func, mpl::index_sequence<type, types...>)
			{
				if (!slots[bucket].template empty<type>())
				{
					const auto index = slots[bucket].template get_index<type>();
					debug_assert(index >= 0);

					auto & array = std::get<type>(arrays);
					debug_assert(index < array.size);

					detail::call_impl_func(std::forward<F>(func), key, array.get(index));
				}
				call_all_impl(key, bucket, std::forward<F>(func), mpl::index_sequence<types...>{});
			}
		};

		/**
		 * \tparam Key                 The identification/lookup key.
		 * \tparam LookupStorageTraits A storage traits for the lookup table.
		 * \tparam ComponentStorages   A storage for each component.
		 *
		 * The unordered collection has the following properties:
		 * - does not move around the components, and
		 * - the components cannot be iterated.
		 *
		 * Note however that components will be moved if reallocation is
		 * necessary (and possible).
		 */
		template <typename Key, typename LookupStorageTraits, typename ...ComponentStorages>
		class UnorderedCollection
		{
#if !(defined(_MSC_VER) && _MSC_VER <= 1926)
			static_assert(mpl::conjunction<mpl::bool_constant<(utility::storage_size<ComponentStorages>::value == 1)>...>::value, "UnorderedCollection does not support multi-type storages for components");
#endif

		private:
			using component_types = mpl::type_list<typename ComponentStorages::template value_type_at<0>...>;

		private:
			using bucket_t = uint32_t;
			using index_t = uint32_t;
			using uint24_t = uint32_t;

			struct slot_t
			{
				uint32_t value;

				uint8_t get_type() const
				{
					return value >> 24;
				}

				uint24_t get_index() const
				{
					return value & 0x00ffffff;
				}

				void set(uint8_t type, std::size_t index)
				{
					debug_assert(index < 0x01000000, "24 bits are not enough");
					value = (uint32_t{type} << 24) | static_cast<uint32_t>(index);
				}

				void set_index(std::size_t index)
				{
					debug_assert(index < 0x01000000, "24 bits are not enough");
					value = (value & 0xff000000) | static_cast<uint32_t>(index);
				}
			};

			struct relocate_rehash
			{
				template <typename Data>
				bool operator () (Data & new_data, Data & old_data)
				{
					const auto new_size = new_data.capacity();
					new_data.storage().memset_fill(new_data.begin_storage(), new_size, ext::byte{});

					const auto old_size = old_data.capacity();
					for (auto i : ranges::index_sequence(old_size))
					{
						if (old_data.storage().data(old_data.begin_storage())[i].second == Key{})
							continue; // empty

						const auto new_bucket = find_empty_bucket(old_data.storage().data(old_data.begin_storage())[i].second, new_data.storage().data(new_data.begin_storage()).second, new_size);
						if (!debug_verify(new_bucket != bucket_t(-1), "collision when reallocating hash"))
							return false; // todo try with bigger allocation?

						using utility::iter_move;
						new_data.storage().data(new_data.begin_storage())[new_bucket] = iter_move(old_data.storage().data(old_data.begin_storage()) + i);
					}
					return true;
				}
			};

		private:
			utility::array<typename LookupStorageTraits::template storage_type<slot_t, Key>, utility::initialize_zero, utility::reserve_nonempty<utility::reserve_power_of_two>::template type, relocate_rehash> lookup_;
			std::tuple<utility::fragmentation<ComponentStorages>...> arrays_;

			decltype(auto) slots() { return lookup_.data().first; }
			decltype(auto) slots() const { return lookup_.data().first; }

			decltype(auto) keys() { return lookup_.data().second; }
			decltype(auto) keys() const { return lookup_.data().second; }

		public:
			template <typename K>
			annotate_nodiscard
			const Key * find_key(K key) const
			{
				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr; // todo weird

				return keys() + bucket;
			}

			template <typename K>
			annotate_nodiscard
			bool contains(K key) const
			{
				return find(key) != bucket_t(-1);
			}

			template <typename C, typename K>
			annotate_nodiscard
			bool contains(K key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return false;

				return slots()[bucket].get_type() == type;
			}

			template <typename C, typename K>
			annotate_nodiscard
			C * try_get(K key)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				if (slots()[bucket].get_type() != type)
					return nullptr;

				const auto index = slots()[bucket].get_index();
				return &std::get<type>(arrays_)[index];
			}

			template <typename C, typename K>
			annotate_nodiscard
			const C * try_get(Key key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				if (slots()[bucket].get_type() != type)
					return nullptr;

				const auto index = slots()[bucket].get_index();
				return &std::get<type>(arrays_)[index];
			}

			annotate_nodiscard
			ext::usize get_all_keys(Key * buffer, ext::usize size) const
			{
				ext::usize count = 0;

				for (auto bucket : ranges::index_sequence(lookup_.size()))
				{
					const auto key = keys()[bucket];
					if (key == Key{})
						continue;

					if (count < size)
					{
						buffer[count] = key;
					}
					count++;
				}
				return count;
			}

			annotate_nodiscard
			constexpr std::size_t max_size() const { return lookup_.size(); } // todo lookup_.max_size()?

			void clear()
			{
				utl::for_each(arrays_, [](auto & array){ array.clear(); });

				keys().memset_fill(0, lookup_.size(), ext::byte{});
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * try_emplace(Key key, Ps && ...ps)
			{
				debug_assert(!contains(key));

				const auto bucket = try_place(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				return add_impl<Component>(bucket, key, std::forward<Ps>(ps)...);
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * try_replace(Key key, Ps && ... ps)
			{
				auto bucket = find(key);
				if (bucket == bucket_t(-1))
				{
					bucket = try_place(key);
					if (bucket == bucket_t(-1))
						return nullptr;
				}
				else
				{
					remove_impl(bucket);
				}

				return add_impl<Component>(bucket, key, std::forward<Ps>(ps)...);
			}

			template <typename K>
			annotate_nodiscard
			bool try_remove(K key)
			{
				const auto bucket = find(key);
				if (bucket == bucket_t(-1))
					return false;

				remove_impl(bucket);

				return true;
			}

			template <typename K, typename F>
			auto call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), std::declval<Key>(), std::declval<mpl::car<component_types> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots()[bucket].get_index();

				switch (slots()[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 keys()[bucket], index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 keys()[bucket], index, std::forward<F>(func));
				}
			}

		private:
			template <typename K>
			static bucket_t find_bucket(K key, const Key * keys, ext::usize nkeys, std::size_t first_bucket)
			{
				auto bucket = first_bucket;
				int count = 0;
				while (keys[bucket] != key)
				{
					if (count >= 4) // arbitrary
						return bucket_t(-1);

					count++;
					bucket = (first_bucket + count * count) % nkeys;
				}
				return static_cast<bucket_t>(bucket);
			}

			template <typename K>
			static bucket_t find_bucket(K key, const Key * keys, ext::usize nkeys)
			{
				if (!debug_assert(nkeys != 0))
					return bucket_t(-1);

				const auto first_bucket = (std::size_t(key) * std::size_t(key)) % nkeys; // todo

				return find_bucket(key, keys, nkeys, first_bucket);
			}

			template <typename K>
			static bucket_t find_empty_bucket(K key, const Key * keys, ext::usize nkeys)
			{
				if (!debug_assert(nkeys != 0))
					return bucket_t(-1);

				const auto first_bucket = (std::size_t(key) * std::size_t(key)) % nkeys; // todo

				return find_bucket(Key{}, keys, nkeys, first_bucket);
			}

			bucket_t try_place(Key key)
			{
				while (true)
				{
					const auto bucket = find_empty_bucket(key, keys(), lookup_.size());
					if (bucket != bucket_t(-1))
						return bucket;

					if (!debug_verify(lookup_.try_reserve(lookup_.capacity() + 1)))
						return bucket_t(-1);
				}
			}

			template <typename K>
			bucket_t find(K key) const
			{
				return find_bucket(key, keys(), lookup_.size());
			}

			template <typename Component, typename ...Ps>
			Component * add_impl(bucket_t bucket, Key key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, component_types>::value;

				auto & array = std::get<type>(arrays_);

				Component * const component = array.try_emplace(std::forward<Ps>(ps)...);
				if (component)
				{
					slots()[bucket].set(type, component - array.data());
					keys()[bucket] = key;
				}
				return component;
			}

			void remove_impl_case(mpl::index_constant<std::size_t(-1)>, bucket_t /*bucket*/, uint24_t /*index*/)
			{
				intrinsic_unreachable();
			}

			template <std::size_t type>
			void remove_impl_case(mpl::index_constant<type>, bucket_t bucket, uint24_t index)
			{
				auto & array = std::get<type>(arrays_);
				if (!debug_assert(index < array.capacity()))
					return;

				keys()[bucket] = Key{};
				debug_verify(array.try_erase(index));
			}

			void remove_impl(bucket_t bucket)
			{
				const auto index = slots()[bucket].get_index();

				switch (slots()[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					remove_impl_case(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, bucket, index); \
					break

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					remove_impl_case(mpl::index_constant<std::size_t(-1)>{}, bucket, index);
				}
			}

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
			// C4702 - unreachable code
#endif
			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, Key key, uint24_t /*index*/, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				intrinsic_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return detail::call_impl_func(std::forward<F>(func), key, *reinterpret_cast<mpl::car<component_types> *>(0));
			}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, Key key, uint24_t index, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				auto & array = std::get<type>(arrays_);

				return detail::call_impl_func(std::forward<F>(func), key, array[index]);
			}
		};
	}
}

#endif /* CORE_CONTAINER_COLLECTION_HPP */
