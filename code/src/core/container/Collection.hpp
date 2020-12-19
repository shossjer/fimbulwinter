
#ifndef CORE_CONTAINER_COLLECTION_HPP
#define CORE_CONTAINER_COLLECTION_HPP

#include "core/debug.hpp"

#include "utility/annotate.hpp"
#include "utility/bitmanip.hpp"
#include "utility/container/array.hpp"
#include "utility/container/fragmentation.hpp"
#include "utility/container/vector.hpp"
#include "utility/overload.hpp"
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

			template <typename Collection>
			class CollectionHandle
			{
				using this_type = CollectionHandle<Collection>;

				friend Collection;

			private:
				uint32_t bucket_; // todo lookup iterator

			public:
				explicit CollectionHandle(uint32_t bucket) : bucket_(bucket) {}

			private:
				friend bool operator == (this_type x, this_type y) { return x.bucket_ == y.bucket_; }
				friend bool operator != (this_type x, this_type y) { return !(x == y); }
			};
		}

		/**
		 * \tparam Key                 The identification/lookup key.
		 * \tparam LookupStorageTraits A storage traits for the lookup table.
		 * \tparam ComponentStorages   A storage for each component.
		 */
		template <typename Key, typename LookupStorageTraits, typename ...ComponentStorages>
		class Collection
		{
#if !defined(_MSC_VER)
			static_assert(mpl::conjunction<mpl::bool_constant<(utility::storage_size<ComponentStorages>::value == 1)>...>::value, "Collection does not support multi-type storages for components");
#endif

			using this_type = Collection<Key, LookupStorageTraits, ComponentStorages...>;

			using component_types = mpl::type_list<typename ComponentStorages::template value_type_at<0>...>;

		public:
			using const_iterator = detail::CollectionHandle<this_type>;
			using iterator = const_iterator;

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
			annotate_nodiscard
			const_iterator end() const { return const_iterator(bucket_t(-1)); }

			template <typename C>
			annotate_nodiscard
			bool contains(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return false;

				return slots()[it.bucket_].get_type() == type;
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

			template <typename C>
			annotate_nodiscard
			C * get(const_iterator it)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].get_type() != type)
					return nullptr;

				const auto index = slots()[it.bucket_].get_index();
				return &std::get<type>(arrays_)[index].first;
			}

			template <typename C>
			annotate_nodiscard
			const C * get(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].get_type() != type)
					return nullptr;

				const auto index = slots()[it.bucket_].get_index();
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

			annotate_nodiscard
			Key get_key(const_iterator it) const
			{
				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return Key{};

				return keys()[it.bucket_];
			}

			annotate_nodiscard
			Key * get_all_keys(Key * buffer, ext::usize size) const
			{
				debug_expression(Key * const buffer_end = buffer + size);

				for (auto it = lookup_.data().second; it != lookup_.data().second + lookup_.size(); ++it)
				{
					const auto key = *it;
					if (key == Key{})
						continue;

					if (debug_assert(buffer != buffer_end))
					{
						*buffer++ = key;
					}
				}
				return buffer;
			}

			annotate_nodiscard
			constexpr std::size_t table_size() const { return lookup_.size(); }

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

				std::memset(keys(), static_cast<int>(ext::byte{}), lookup_.size() * sizeof(Key));
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * emplace(Key key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, component_types>::value;

				if (!debug_assert(find(key) == bucket_t(-1)))
					return nullptr;

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

			void erase(const_iterator it)
			{
				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return;

				const auto index = slots()[it.bucket_].get_index();

				switch (slots()[it.bucket_].get_type())
				{
#define CASE(n) case (n):	  \
					remove_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, it.bucket_, index); \
					break

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					intrinsic_unreachable();
				}
			}

			template <typename F>
			auto call(const_iterator it, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), std::declval<Key>(), std::declval<mpl::car<component_types> &>()))
			{
				debug_assert(it.bucket_ != bucket_t(-1));

				const auto index = slots()[it.bucket_].get_index();

				switch (slots()[it.bucket_].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 keys()[it.bucket_], index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 keys()[it.bucket_], index, std::forward<F>(func));
				}
			}

			template <typename ...Fs>
			decltype(auto) call(const_iterator it, Fs && ...funcs) { return call(it, ext::overload(std::forward<Fs>(funcs)...)); }

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

			template <typename K>
			annotate_nodiscard
			friend const_iterator find(const this_type & x, K key) { return const_iterator(x.find(key)); }
		};

		/**
		 * \tparam Key                 The identification/lookup key.
		 * \tparam LookupStorageTraits A storage traits for the lookup table.
		 * \tparam ComponentStorages   A storage for each component.
		 *
		 * The multi collection has the following properties:
		 * - it can hold at most one component of each type for a given key
		 */
		template <typename Key, typename LookupStorageTraits, typename ...ComponentStorages>
		class MultiCollection
		{
#if !defined(_MSC_VER)
			static_assert(mpl::conjunction<mpl::bool_constant<(utility::storage_size<ComponentStorages>::value == 1)>...>::value, "MultiCollection does not support multi-type storages for components");
#endif

			using this_type = MultiCollection<Key, LookupStorageTraits, ComponentStorages...>;

			using component_types = mpl::type_list<typename ComponentStorages::template value_type_at<0>...>;

		public:
			using const_iterator = detail::CollectionHandle<this_type>;
			using iterator = const_iterator;

		private:
			using bucket_t = uint32_t;

		private:
			struct slot_t
			{
				static_assert(component_types::size <= 64, "the type mask only supports at most 64 types");

				uint64_t type_mask;
				uint16_t indices[component_types::size];

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
			std::tuple<utility::vector<typename utility::storage_traits<ComponentStorages>::template append<Key>>...> arrays_;

			decltype(auto) slots() { return lookup_.data().first; }
			decltype(auto) slots() const { return lookup_.data().first; }

			decltype(auto) keys() { return lookup_.data().second; }
			decltype(auto) keys() const { return lookup_.data().second; }

		public:
			annotate_nodiscard
			const_iterator end() const { return const_iterator(bucket_t(-1)); }

			template <typename C>
			annotate_nodiscard
			bool contains(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return false;

				return !slots()[it.bucket_].template empty<type>();
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

			template <typename C>
			annotate_nodiscard
			C * get(const_iterator it)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].template empty<type>())
					return nullptr;

				const auto index = slots()[it.bucket_].template get_index<type>();
				return &std::get<type>(arrays_)[index].first;
			}

			template <typename C>
			annotate_nodiscard
			const C * get(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].template empty<type>())
					return nullptr;

				const auto index = slots()[it.bucket_].template get_index<type>();
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

			annotate_nodiscard
			Key get_key(const_iterator it) const
			{
				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return Key{};

				return keys()[it.bucket_];
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * emplace(Key key, Ps && ...ps)
			{
				constexpr auto type = mpl::index_of<Component, component_types>::value;

				const auto bucket = try_place(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				if (!debug_assert(slots()[bucket].template empty<type>()))
					return nullptr;

				auto & array = std::get<type>(arrays_);
				const auto index = array.size();

				if (!array.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(std::forward<Ps>(ps)...), std::forward_as_tuple(key)))
					return nullptr;

				slots()[bucket].template set<type>(debug_cast<uint16_t>(index));
				keys()[bucket] = key;

				return &array[index].first;
			}

			template <typename C>
			void erase(const_iterator it)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return;

				if (!debug_assert(!slots()[it.bucket_].template empty<type>()))
					return;

				remove_at_impl<type>(it.bucket_, slots()[it.bucket_].template get_index<type>());
			}

			void erase(const_iterator it)
			{
				if (debug_assert(it.bucket_ != bucket_t(-1)))
				{
					remove_impl(it.bucket_, mpl::make_index_sequence<component_types::size>{});
				}
			}

			template <typename F>
			void call(const_iterator it, F && func)
			{
				if (debug_assert(it.bucket_ != bucket_t(-1)))
				{
					call_all_impl(it.bucket_, std::forward<F>(func), mpl::make_index_sequence<component_types::size>{});
				}
			}

			template <typename ...Fs>
			decltype(auto) call(const_iterator it, Fs && ...funcs) { return call(it, ext::overload(std::forward<Fs>(funcs)...)); }
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
				auto bucket = find_bucket(key, keys(), lookup_.size());
				if (bucket != bucket_t(-1))
					return bucket;

				while (true)
				{
					bucket = find_empty_bucket(key, keys(), lookup_.size());
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

			template <std::size_t type>
			void remove_at_impl(bucket_t bucket, uint16_t index)
			{
				auto & array = std::get<type>(arrays_);
				debug_assert(index < array.size());

				const auto last_index = array.size() - 1;
				const auto last_bucket = find(array[last_index].second);

				slots()[last_bucket].template set_index<type>(index);
				slots()[bucket].template clear<type>();
				if (slots()[bucket].empty())
				{
					keys()[bucket] = Key{};
				}
				array.erase(array.begin() + index);
			}

			void remove_impl(bucket_t /*bucket*/, mpl::index_sequence<>)
			{}
			template <size_t type, size_t ...types>
			void remove_impl(bucket_t bucket, mpl::index_sequence<type, types...>)
			{
				if (!slots()[bucket].template empty<type>())
				{
					const auto index = slots()[bucket].template get_index<type>();

					remove_at_impl<type>(bucket, index);
				}
				remove_impl(bucket, mpl::index_sequence<types...>{});
			}

			template <typename F>
			void call_all_impl(bucket_t /*bucket*/, F && /*func*/, mpl::index_sequence<>)
			{}
			template <typename F, size_t type, size_t ...types>
			void call_all_impl(bucket_t bucket, F && func, mpl::index_sequence<type, types...>)
			{
				if (!slots()[bucket].template empty<type>())
				{
					const auto index = slots()[bucket].template get_index<type>();

					auto & array = std::get<type>(arrays_);
					debug_assert(index < array.size());

					detail::call_impl_func(std::forward<F>(func), keys()[bucket], array[index].first);
				}
				call_all_impl(bucket, std::forward<F>(func), mpl::index_sequence<types...>{});
			}

			template <typename K>
			annotate_nodiscard
			friend const_iterator find(const this_type & x, K key) { return const_iterator(x.find(key)); }
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
#if !defined(_MSC_VER)
			static_assert(mpl::conjunction<mpl::bool_constant<(utility::storage_size<ComponentStorages>::value == 1)>...>::value, "UnorderedCollection does not support multi-type storages for components");
#endif

			using this_type = UnorderedCollection<Key, LookupStorageTraits, ComponentStorages...>;

			using component_types = mpl::type_list<typename ComponentStorages::template value_type_at<0>...>;

		public:
			using const_iterator = detail::CollectionHandle<this_type>;
			using iterator = const_iterator;

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
			annotate_nodiscard
			const_iterator end() const { return const_iterator(bucket_t(-1)); }

			template <typename C>
			annotate_nodiscard
			bool contains(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return false;

				return slots()[it.bucket_].get_type() == type;
			}

			template <typename C>
			annotate_nodiscard
			C * get(const_iterator it)
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].get_type() != type)
					return nullptr;

				const auto index = slots()[it.bucket_].get_index();
				return &std::get<type>(arrays_)[index];
			}

			template <typename C>
			annotate_nodiscard
			const C * get(const_iterator it) const
			{
				constexpr auto type = mpl::index_of<C, component_types>::value;

				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return nullptr;

				if (slots()[it.bucket_].get_type() != type)
					return nullptr;

				const auto index = slots()[it.bucket_].get_index();
				return &std::get<type>(arrays_)[index];
			}

			annotate_nodiscard
			Key get_key(const_iterator it) const
			{
				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return Key{};

				return keys()[it.bucket_];
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

				std::memset(keys(), static_cast<int>(ext::byte{}), lookup_.size() * sizeof(Key));
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * emplace(Key key, Ps && ...ps)
			{
				if (!debug_assert(find(key) == bucket_t(-1)))
					return nullptr;

				const auto bucket = try_place(key);
				if (bucket == bucket_t(-1))
					return nullptr;

				return add_impl<Component>(bucket, key, std::forward<Ps>(ps)...);
			}

			template <typename Component, typename ...Ps>
			annotate_nodiscard
			Component * replace(Key key, Ps && ... ps)
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

			void erase(const_iterator it)
			{
				if (!debug_assert(it.bucket_ != bucket_t(-1)))
					return;

				remove_impl(it.bucket_);
			}

			template <typename F>
			auto call(const_iterator it, F && func) ->
				decltype(detail::call_impl_func(std::forward<F>(func), std::declval<Key>(), std::declval<mpl::car<component_types> &>()))
			{
				debug_assert(it.bucket_ != bucket_t(-1));

				const auto index = slots()[it.bucket_].get_index();

				switch (slots()[it.bucket_].get_type())
				{
#define CASE(n) case (n):	  \
					return call_impl(mpl::index_constant<((n) < component_types::size ? (n) : std::size_t(-1))>{}, \
					                 keys()[it.bucket_], index, std::forward<F>(func))

					PP_EXPAND_128(CASE, 0);
#undef CASE
				default:
					return call_impl(mpl::index_constant<std::size_t(-1)>{},
					                 keys()[it.bucket_], index, std::forward<F>(func));
				}
			}

			template <typename ...Fs>
			decltype(auto) call(const_iterator it, Fs && ...funcs) { return call(it, ext::overload(std::forward<Fs>(funcs)...)); }

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

			template <typename K>
			annotate_nodiscard
			friend const_iterator find(const this_type & x, K key) { return const_iterator(x.find(key)); }
		};
	}
}

#endif /* CORE_CONTAINER_COLLECTION_HPP */
