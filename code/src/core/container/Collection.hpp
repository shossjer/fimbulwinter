
#ifndef CORE_CONTAINER_COLLECTION_HPP
#define CORE_CONTAINER_COLLECTION_HPP

#include <core/debug.hpp>

#include <utility/array_alloc.hpp>
#include <utility/bitmanip.hpp>
#include <utility/preprocessor.hpp>
#include <utility/type_traits.hpp>
#include <utility/utility.hpp>

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
			auto call_impl_func(F && func, K key) ->
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
			template <typename F, typename K, typename P>
			auto call_impl_func(F && func, K key, P && p) ->
				decltype(func(std::forward<P>(p)))
			{
				return func(std::forward<P>(p));
			}


			using bucket_t = uint32_t;

			template <typename StorageTraits, typename C>
			class bucket_array_t
			{
			private:
				utility::array_wrapper<utility::array_data<typename StorageTraits::template storage_type<C, bucket_t>>> data_;

			public:
				C * begin() { return components().data(); }
				const C * begin() const { return components().data(); }
				C * end() { return components().data() + data_.size(); }
				const C * end() const { return components().data() + data_.size(); }

				C & get(std::ptrdiff_t index) { return components()[index]; }
				const C & get(std::ptrdiff_t index) const { return components()[index]; }

				bucket_t bucket_at(std::ptrdiff_t index) const { return buckets()[index]; }

				constexpr std::size_t capacity() const { return data_.capacity(); }
				std::size_t size() const { return data_.size(); }

				void clear()
				{
					data_.destruct_range(0, data_.size());
					data_.set_size(0);
				}

				template <typename ...Ps>
				void emplace_back(bucket_t bucket, Ps && ...ps)
				{
					data_.grow();

					components().construct_at(data_.size(), std::forward<Ps>(ps)...);
					buckets().construct_at(data_.size(), bucket);
					data_.set_size(data_.size() + 1);
				}

				void remove_at(std::ptrdiff_t index)
				{
					debug_assert((0 <= index && index < data_.size()));

					const auto last = data_.size() - 1;

					components()[index] = std::move(components()[last]);
					buckets()[index] = std::move(buckets()[last]);
					data_.destruct_range(last, data_.size());
					data_.set_size(last);
				}
			private:
				decltype(auto) components() { return data_.storage_.section(mpl::index_constant<0>{}, data_.capacity()); }
				decltype(auto) components() const { return data_.storage_.section(mpl::index_constant<0>{}, data_.capacity()); }

				decltype(auto) buckets() { return data_.storage_.section(mpl::index_constant<1>{}, data_.capacity()); }
				decltype(auto) buckets() const { return data_.storage_.section(mpl::index_constant<1>{}, data_.capacity()); }
			};
		}

		/**
		 * \tparam Key      The identification/lookup key.
		 * \tparam Maximum  Should be about twice as many as is needed.
		 * \tparam Storages A storage (like static_array_storage) for each type to store.
		 */
		template <typename Key, std::size_t Maximum, typename ...Storages>
		class Collection
		{
		private:
			using component_types = mpl::type_list<typename Storages::template value_type_at<0>...>;
			template <typename Storage>
			using storage_traits = utility::storage_traits<Storage>;

		private:
			using bucket_t = detail::bucket_t;
			using uint24_t = uint32_t;

		private:
			struct slot_t
			{
				uint32_t value;

				slot_t() :
					value(0xffffffff)
				{}

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
			std::tuple<detail::bucket_array_t<storage_traits<Storages>, typename Storages::template value_type_at<0>>...> arrays;
			slot_t slots[Maximum];
			Key keys[Maximum];

		public:
			bool contains(Key key) const
			{
				return try_find(key) != bucket_t(-1);
			}
			template <typename C>
			bool contains(Key key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return false;

				return slots[bucket].get_type() == type;
			}
			template <typename C>
			decltype(auto) get()
			{
				return std::get<mpl::index_of<C, component_types>::value>(arrays);
			}
			template <typename C>
			decltype(auto) get() const
			{
				return std::get<mpl::index_of<C, component_types>::value>(arrays);
			}
			template <typename C>
			C & get(Key key)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				debug_assert(slots[bucket].get_type() == type);
				const auto index = slots[bucket].get_index();

				return std::get<type>(arrays).get(index);
			}
			template <typename C>
			const C & get(Key key) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto bucket = find(key);
				debug_assert(slots[bucket].get_type() == type);
				const auto index = slots[bucket].get_index();

				return std::get<type>(arrays).get(index);
			}

			template <typename C>
			Key get_key(const C & component) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				const auto & array = std::get<type>(arrays);
				debug_assert(&component >= array.begin());
				debug_assert(&component < array.end());

				return keys[array.bucket_at(std::distance(array.begin(), &component))];
			}

			template <typename D>
			void add(Key key, D && data)
			{
				using Components = mpl::type_filter<std::is_constructible,
				                                    component_types,
				                                    D &&>;
				static_assert(Components::size == 1, "Exactly one type needs to be constructible with the argument.");

				emplace<mpl::car<Components>>(key, std::forward<D>(data));
			}
			void clear()
			{
				clear_all_impl(mpl::make_index_sequence<component_types::size>{});
			}
			template <typename Component, typename ...Ps>
			Component & emplace(Key key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, component_types>::value;

				debug_assert(!contains(key));

				const auto bucket = place(key);
				auto & array = std::get<type>(arrays);

				const auto index = array.size();

				// todo: if we can fail gracefully from
				// array.emplace_back, we should not leave slots and keys
				// modified
				slots[bucket].set(type, index);
				keys[bucket] = key;
				array.emplace_back(bucket, std::forward<Ps>(ps)...);

				return array.get(index);
			}
			void remove(Key key)
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
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

			template <typename F>
			auto call(Key key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 key, index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 key, index, std::forward<F>(func));
				}
			}

			template <typename F>
			auto try_call(Key key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<component_types> &>()))
			{
				const auto bucket = try_find(key);

				if (bucket == bucket_t(-1)) return detail::call_impl_func(std::forward<F>(func), key);

				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 key, index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 key, index, std::forward<F>(func));
				}
			}
		private:
			// not great
			bucket_t hash(Key key) const
			{
				return (std::size_t(key) * std::size_t(key)) % Maximum;
			}
			/**
			 * Find an empty bucket where the key can be placed.
			 */
			bucket_t place(Key key)
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (!slots[bucket].empty()) // ... this bucket is not empty!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= Maximum - 1)
						bucket -= Maximum;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t find(Key key) const
			{
				auto bucket = hash(key);
				std::size_t count = 0; // debug count that asserts if taken too many steps
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					debug_assert(count++ < std::size_t{4});
					if (bucket++ >= Maximum - 1)
						bucket -= Maximum;
				}
				return bucket;
			}
			/**
			 * Find the bucket where the key resides.
			 */
			bucket_t try_find(Key key) const
			{
				auto bucket = hash(key);
				std::size_t count = 0;
				// search again if...
				while (slots[bucket].empty() || // ... this bucket is empty, or
				       keys[bucket] != key) // ... this is not the right one!
				{
					if (count++ >= 4)
						return bucket_t(-1);
					if (bucket++ >= Maximum - 1)
						bucket -= Maximum;
				}
				return bucket;
			}

			void clear_all_impl(mpl::index_sequence<>)
			{
			}
			template <size_t type, size_t ...types>
			void clear_all_impl(mpl::index_sequence<type, types...>)
			{
				auto & array = std::get<type>(arrays);
				for (int i = 0; i < array.size(); i++)
				{
					slots[array.bucket_at(i)].clear();
				}
				array.clear();

				clear_all_impl(mpl::index_sequence<types...>{});
			}
			void remove_impl(mpl::index_constant<std::size_t(-1)>, bucket_t bucket, uint24_t index)
			{
				intrinsic_unreachable();
			}
			template <std::size_t type>
			void remove_impl(mpl::index_constant<type>, bucket_t bucket, uint24_t index)
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size());

				const auto last = array.size() - 1;

				slots[array.bucket_at(last)].set_index(index);
				slots[bucket].clear();
				// keys[bucket] = ??? // not needed
				array.remove_at(index);
			}

			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, Key key, uint24_t index, F && func) ->
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
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.size());

				return detail::call_impl_func(std::forward<F>(func), key, array.get(index));
			}
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
				utility::static_storage<N, C> components;
				bucket_t buckets[N];

				C * begin() { return components.data(); }
				const C * begin() const { return components.data(); }
				C * end() { return components.data() + size; }
				const C * end() const { return components.data() + size; }

				C & get(const std::size_t index) { return components[index]; }
				const C & get(const std::size_t index) const { return components[index]; }

				template <typename ...Ps>
				void construct(const std::size_t index, Ps && ...ps) { (void)components.construct_at(index, std::forward<Ps>(ps)...); }
				void destruct(const std::size_t index) { components.destruct_at(index); }
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
			bool contains(K key) const
			{
				return try_find(key) != bucket_t(-1);
			}
			template <typename C>
			bool contains(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return false;

				return !slots[bucket].template empty<type>();
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
			C & get(K key)
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				return std::get<type>(arrays).get(index);
			}
			template <typename C>
			const C & get(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = find(key);
				const auto index = slots[bucket].template get_index<type>();
				debug_assert(index >= 0);

				return std::get<type>(arrays).get(index);
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
				int count = 0; // debug count that asserts if taken too many steps
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
				int count = 0; // debug count that asserts if taken too many steps
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
			void remove_at_impl(bucket_t bucket, uint32_t index)
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

			void remove_impl(bucket_t bucket, mpl::index_sequence<>)
			{
			}
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

			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, K key, bucket_t bucket, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				intrinsic_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return detail::call_impl_func(std::forward<F>(func), key, *reinterpret_cast<mpl::car<Cs...> *>(0));
			}
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
			void call_all_impl(K key, bucket_t bucket, F && func, mpl::index_sequence<>)
			{
			}
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

				std::size_t size = 0;
				utility::static_storage<N, C> components;
				uint24_t free_indices[N];

				array_t()
				{
					std::iota(free_indices + 0, free_indices + N, 0);
				}

				C & get(const std::size_t index) { return components[index]; }
				const C & get(const std::size_t index) const { return components[index]; }

				template <typename ...Ps>
				void construct(const std::size_t index, Ps && ...ps) { (void)components.construct_at(index, std::forward<Ps>(ps)...); }
				void destruct(const std::size_t index) { components.destruct_at(index); }
			};
		private:
			struct slot_t
			{
				uint32_t value;

				slot_t() :
					value(0xffffffff)
				{}

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
			bool contains(K key) const
			{
				return try_find(key) != bucket_t(-1);
			}
			template <typename C>
			bool contains(K key) const
			{
				constexpr auto type = mpl::index_of<C, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return false;

				return slots[bucket].get_type() == type;
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
			int get_all_keys(K * buffer, int size) const
			{
				int count = 0;
				if (size <= 0)
					return count;

				for (int bucket = 0; bucket < M; bucket++)
				{
					if (slots[bucket].empty())
						continue;

					buffer[count++] = keys[bucket];
					if (count >= size)
						return count;
				}
				return count;
			}
			constexpr std::size_t max_size() const { return M; }

			template <typename D>
			void add(K key, D && data)
			{
				using Components = mpl::type_filter<std::is_constructible,
				                                    mpl::type_list<Cs...>,
				                                    D &&>;
				static_assert(Components::size == 1, "Exactly one type needs to be constructible with the argument.");

				emplace<mpl::car<Components>>(key, std::forward<D>(data));
			}
			void clear()
			{
				clear_all_impl(mpl::make_index_sequence<sizeof...(Cs)>{});

				for (auto & slot : slots)
				{
					slot.clear();
				}
			}
			template <typename Component, typename ...Ps>
			Component & emplace(K key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, mpl::type_list<Cs...>>::value;

				debug_assert(!contains(key));

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

				remove_component(bucket);
				slots[bucket].clear();
			}
			template <typename Component, typename ...Ps>
			Component & replace(K key, Ps && ... ps)
			{
				constexpr auto type = mpl::index_of<Component, mpl::type_list<Cs...>>::value;

				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return emplace<Component>(key, std::forward<Ps>(ps)...);

				remove_component(bucket);

				auto & array = std::get<type>(arrays);
				debug_assert(array.size < array.capacity);
				const auto index = array.free_indices[array.size];

				slots[bucket].set(type, index);
				array.construct(index, std::forward<Ps>(ps)...);
				array.size++;

				return array.get(index);
			}
			void try_remove(K key)
			{
				const auto bucket = try_find(key);
				if (bucket == bucket_t(-1))
					return;

				remove_component(bucket);
				slots[bucket].clear();
			}

			template <typename F>
			auto call(K key, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				const auto bucket = find(key);
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, \
					                 key, index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 key, index, std::forward<F>(func));
				}
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
			bucket_t find(K key) const
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
			bucket_t try_find(K key) const
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

			void clear_all_impl(mpl::index_sequence<>)
			{
			}
			template <size_t type, size_t ...types>
			void clear_all_impl(mpl::index_sequence<type, types...>)
			{
				auto & array = std::get<type>(arrays);
				{
					uint24_t busy_indices[array.capacity];
					std::iota(busy_indices + 0, busy_indices + array.capacity, 0);
					for (int i = array.size; i < array.capacity; i++)
					{
						busy_indices[array.free_indices[i]] = -1;
					}
					for (const auto busy_index : busy_indices)
					{
						if (busy_index == -1)
							continue;

						array.destruct(busy_index);
					}
				}
				array.size = 0;

				clear_all_impl(mpl::index_sequence<types...>{});
			}
			void remove_component_impl(mpl::index_constant<std::size_t(-1)>, uint24_t index)
			{
				intrinsic_unreachable();
			}
			template <std::size_t type>
			void remove_component_impl(mpl::index_constant<type>, uint24_t index)
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.capacity);

				array.destruct(index);
				array.free_indices[array.size - 1] = index;
				array.size--;
			}
			void remove_component(bucket_t bucket)
			{
				const auto index = slots[bucket].get_index();

				switch (slots[bucket].get_type())
				{
#define CASE(n) case (n):	  \
					remove_component_impl(mpl::index_constant<((n) < sizeof...(Cs) ? (n) : std::size_t(-1))>{}, index); \
					break

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					intrinsic_unreachable();
				}
			}

			template <typename F>
			auto call_impl(mpl::index_constant<std::size_t(-1)>, K key, uint24_t index, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				intrinsic_unreachable();
				// this is used to deduce the return type correctly
				// we should never get here
				return detail::call_impl_func(std::forward<F>(func), key, *reinterpret_cast<mpl::car<Cs...> *>(0));
			}
			template <std::size_t type, typename F>
			auto call_impl(mpl::index_constant<type>, K key, uint24_t index, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), key, std::declval<mpl::car<Cs...> &>()))
			{
				auto & array = std::get<type>(arrays);
				debug_assert(index < array.capacity);

				return detail::call_impl_func(std::forward<F>(func), key, array.get(index));
			}
		};
	}
}

#endif /* CORE_CONTAINER_COLLECTION_HPP */
