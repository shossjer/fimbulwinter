#pragma once

#include "config.h"

#include "utility/aggregation_allocator.hpp"
#include "utility/algorithm.hpp"
#include "utility/annotate.hpp"
#include "utility/bitmanip.hpp"
#include "utility/compound.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/heap_allocator.hpp"
#include "utility/iterator.hpp"
#include "utility/null_allocator.hpp"
#include "utility/storing.hpp"
#include "utility/tuple.hpp"

#include <array>
#include <cassert>
#include <cstring>

namespace utility
{
	struct zero_initialize_t
	{
		explicit zero_initialize_t() = default;
	};
	constexpr zero_initialize_t zero_initialize = zero_initialize_t{};

	template <std::size_t Capacity, typename ...Ts>
	class static_storage_impl
	{
		using this_type = static_storage_impl<Capacity, Ts...>;

	public:
		using value_type = utility::combine<utility::compound, Ts...>;
		using allocator_type = utility::null_allocator<char>;
		using size_type = std::size_t; // todo size_for<Capacity>?
		using difference_type = std::ptrdiff_t;
		using reference = utility::combine<utility::compound, Ts &...>;
		using const_reference = utility::combine<utility::compound, const Ts &...>;
		using rvalue_reference = utility::combine<utility::compound, Ts &&...>;
		using const_rvalue_reference = utility::combine<utility::compound, const Ts &&...>;
		using pointer = utility::combine<utility::zip_pointer, Ts *...>;
		using const_pointer = utility::combine<utility::zip_pointer, const Ts *...>;

		template <std::size_t I>
		using value_type_at = mpl::type_at<I, Ts...>; // todo remove
		template <typename Storing>
		using value_type_for = typename Storing::value_type;
		template <typename T>
		using storing_type_for = utility::storing<T>;

		using iterator = utility::combine<utility::zip_iterator, storing_type_for<Ts> *...>;
		using const_iterator = utility::combine<utility::zip_iterator, const storing_type_for<Ts> *...>;
		using position = storing_type_for<mpl::car<Ts...>> *;
		using const_position = const storing_type_for<mpl::car<Ts...>> *;

		using data_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<utility::storing<Ts>>...>;

		using storing_trivially_copyable =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_copyable,
			                          utility::storing<Ts>...>>;
		using storing_trivially_destructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_destructible,
			                          utility::storing<Ts>...>>;
		using storing_trivially_default_constructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_default_constructible,
			                          utility::storing<Ts>...>>;

	private:
		utility::tuple<std::array<storing_type_for<Ts>, Capacity>...> arrays;

	public:
		annotate_nodiscard
		bool allocate(std::size_t capacity)
		{
			return /*debug_assert*/(capacity == Capacity);
		}

		annotate_nodiscard
		constexpr bool good() const
		{
			return true;
		}

		void deallocate(std::size_t capacity)
		{
			assert(capacity == Capacity);
			static_cast<void>(capacity);
		}

		annotate_nodiscard
		constexpr std::size_t max_size() const { return Capacity; }

		template <typename T, typename ...Ps>
		T & construct_at(utility::storing<T> * ptr_, Ps && ...ps)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return ptr_->construct(std::forward<Ps>(ps)...);
		}

		template <typename T>
		void destruct_at(utility::storing<T> * ptr_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			ptr_->destruct();
		}

		template <typename T>
		annotate_nodiscard
		T * data(utility::storing<T> * data_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return &data_->value;
		}

		template <typename T>
		annotate_nodiscard
		const T * data(const utility::storing<T> * data_) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return &data_->value;
		}

		template <typename ...Us,
		          typename Pointer = utility::zip_pointer<Us *...>>
		annotate_nodiscard
		Pointer data(utility::zip_iterator<utility::storing<Us> *...> it)
		{
			return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
		}

		template <typename ...Us,
		          typename Pointer = utility::zip_pointer<const Us *...>>
		annotate_nodiscard
		Pointer data(utility::zip_iterator<const utility::storing<Us> *...> it) const
		{
			return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
		}

		template <typename T>
		annotate_nodiscard
		utility::storing<T> * iter(T * ptr)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return reinterpret_cast<utility::storing<T> *>(ptr); // todo probably okay
		}

		template <typename T>
		annotate_nodiscard
		const utility::storing<T> * iter(const T * ptr) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return reinterpret_cast<const utility::storing<T> *>(ptr); // todo probably okay
		}

		template <typename ...Us,
		          typename Iterator = utility::zip_iterator<utility::storing<Us> *...>>
		annotate_nodiscard
		Iterator iter(utility::zip_pointer<Us *...> ptr)
		{
			return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
		}

		template <typename ...Us,
		          typename Iterator = utility::zip_iterator<const utility::storing<Us> *...>>
		annotate_nodiscard
		Iterator iter(utility::zip_pointer<const Us *...> ptr) const
		{
			return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
		}

		template <std::size_t ...Is,
		          typename Iterator = utility::combine<utility::zip_iterator,
		                                               storing_type_for<mpl::type_at<Is, Ts...>> *...>>
		annotate_nodiscard
		Iterator begin_for(mpl::index_sequence<Is...>)
		{
			return Iterator(get<Is>(arrays).data()...);
		}

		template <std::size_t ...Is,
		          typename Iterator = utility::combine<utility::zip_iterator,
		                                               const storing_type_for<mpl::type_at<Is, Ts...>> *...>>
		annotate_nodiscard
		Iterator begin_for(mpl::index_sequence<Is...>) const
		{
			return Iterator(get<Is>(arrays).data()...);
		}

		annotate_nodiscard
		auto begin() { return begin_for(mpl::make_index_sequence_for<Ts...>{}); }
		annotate_nodiscard
		auto begin() const { return begin_for(mpl::make_index_sequence_for<Ts...>{}); }

		annotate_nodiscard
		auto begin(std::size_t /*capacity*/) { return begin(); }
		annotate_nodiscard
		auto begin(std::size_t /*capacity*/) const { return begin(); }

		template <std::size_t ...Is,
		          typename Iterator = utility::combine<utility::zip_iterator,
		                                               storing_type_for<mpl::type_at<Is, Ts...>> *...>>
		annotate_nodiscard
		Iterator end_for(mpl::index_sequence<Is...>)
		{
			return Iterator(get<Is>(arrays).data() + Capacity...);
		}

		template <std::size_t ...Is,
		          typename Iterator = utility::combine<utility::zip_iterator,
		                                               const storing_type_for<mpl::type_at<Is, Ts...>> *...>>
		annotate_nodiscard
		Iterator end_for(mpl::index_sequence<Is...>) const
		{
			return Iterator(get<Is>(arrays).data() + Capacity...);
		}

		annotate_nodiscard
		auto end() { return end_for(mpl::make_index_sequence_for<Ts...>{}); }
		annotate_nodiscard
		auto end() const { return end_for(mpl::make_index_sequence_for<Ts...>{}); }

		annotate_nodiscard
		auto end(std::size_t /*capacity*/) { return end(); }
		annotate_nodiscard
		auto end(std::size_t /*capacity*/) const { return end(); }

		annotate_nodiscard
		position place(std::size_t index)
		{
			return begin_for(mpl::index_sequence<0>{}) + index;
		}

		annotate_nodiscard
		const_position place(std::size_t index) const
		{
			return begin_for(mpl::index_sequence<0>{}) + index;
		}

		template <typename Position,
		          REQUIRES((std::is_convertible<Position, const_position>::value))>
		annotate_nodiscard
		std::ptrdiff_t index_of(Position pos) const
		{
			return static_cast<const_position>(pos) - begin_for(mpl::index_sequence<0>{});
		}

		template <typename Iterator,
		          REQUIRES((!std::is_convertible<Iterator, const_position>::value))>
		annotate_nodiscard
		std::ptrdiff_t index_of(Iterator it) const
		{
			return index_of(utility::select_first<sizeof...(Ts)>(it));
		}
	};

	template <template <typename> class Allocator, typename ...Ts>
	class dynamic_storage_impl
	{
		using this_type = dynamic_storage_impl<Allocator, Ts...>;

	public:
		using value_type = utility::combine<utility::compound, Ts...>;
		using allocator_type = utility::aggregation_allocator<Allocator, void, Ts...>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = utility::combine<utility::compound, Ts &...>;
		using const_reference = utility::combine<utility::compound, const Ts &...>;
		using rvalue_reference = utility::combine<utility::compound, Ts &&...>;
		using const_rvalue_reference = utility::combine<utility::compound, const Ts &&...>;
		using pointer = utility::combine<utility::zip_pointer, Ts *...>;
		using const_pointer = utility::combine<utility::zip_pointer, const Ts *...>;

		template <std::size_t I>
		using value_type_at = mpl::type_at<I, Ts...>; // todo remove
		template <typename Storing>
		using value_type_for = Storing;
		template <typename T>
		using storing_type_for = T;

		using iterator = utility::combine<utility::zip_iterator, Ts *...>;
		using const_iterator = utility::combine<utility::zip_iterator, const Ts *...>;
		using position = mpl::car<Ts...> *;
		using const_position = const mpl::car<Ts...> *;

		using data_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<Ts>...>;

		using storing_trivially_copyable =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_copyable,
			                          Ts...>>;
		using storing_trivially_destructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_destructible,
			                          Ts...>>;
		using storing_trivially_default_constructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_default_constructible,
			                          Ts...>>;

	private:
		using allocator_traits = std::allocator_traits<allocator_type>;

	private:
		struct empty_allocator_hack : allocator_type
		{
#if MODE_DEBUG
			void * storage_ = nullptr;
#else
			void * storage_;
#endif

#if MODE_DEBUG
			~empty_allocator_hack()
			{
				assert(!storage_);
			}
#endif
			empty_allocator_hack() = default;
			empty_allocator_hack(empty_allocator_hack && other)
				: storage_(std::exchange(other.storage_, nullptr))
			{}
			empty_allocator_hack & operator = (empty_allocator_hack && other)
			{
#if MODE_DEBUG
				assert(!storage_);
#endif
				storage_ = std::exchange(other.storage_, nullptr);

				return *this;
			}
		} impl_;

	public:
		annotate_nodiscard
		bool allocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(!storage());
#endif
			storage() = allocator_traits::allocate(allocator(), capacity);
			return good();
		}

		annotate_nodiscard
		bool good() const
		{
			return storage() != nullptr;
		}

		void deallocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::deallocate(allocator(), storage(), capacity);
#if MODE_DEBUG
			storage() = nullptr;
#endif
		}

		template <typename T, typename ...Ps>
		T & construct_at(T * ptr_, Ps && ...ps)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::construct(allocator(), ptr_, std::forward<Ps>(ps)...);
			return *ptr_;
		}

		template <typename T>
		void destruct_at(T * ptr_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::destroy(allocator(), ptr_);
		}

		template <typename T>
		annotate_nodiscard
		T * data(T * data_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return data_;
		}

		template <typename T>
		annotate_nodiscard
		const T * data(const T * data_) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return data_;
		}

		template <typename ...Us,
		          typename Pointer = utility::zip_pointer<Us *...>>
		annotate_nodiscard
		Pointer data(utility::zip_iterator<Us *...> it)
		{
			return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
		}

		template <typename ...Us,
		          typename Pointer = utility::zip_pointer<const Us *...>>
		annotate_nodiscard
		Pointer data(utility::zip_iterator<const Us *...> it) const
		{
			return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
		}

		template <typename T>
		annotate_nodiscard
		T * iter(T * ptr)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return ptr;
		}

		template <typename T>
		annotate_nodiscard
		const T * iter(const T * ptr) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return ptr;
		}

		template <typename ...Us,
		          typename Iterator = utility::zip_iterator<Us *...>>
		annotate_nodiscard
		Iterator iter(utility::zip_pointer<Us *...> ptr)
		{
			return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
		}

		template <typename ...Us,
		          typename Iterator = utility::zip_iterator<const Us *...>>
		annotate_nodiscard
		Iterator iter(utility::zip_pointer<const Us *...> ptr) const
		{
			return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
		}

		annotate_nodiscard
		mpl::car<Ts...> * begin_for(mpl::index_sequence<0>)
		{
			// note there are times when we will call allocator address
			// with a garbage storage, but maybe that is okay since then
			// the capacity must be zero and any access through the
			// returned address is undefined :unicorn:
			return allocator().template address<0>(storage(), 0);
		}

		annotate_nodiscard
		const mpl::car<Ts...> * begin_for(mpl::index_sequence<0>) const
		{
			// note there are times when we will call allocator address
			// with a garbage storage, but maybe that is okay since then
			// the capacity must be zero and any access through the
			// returned address is undefined :unicorn:
			return allocator().template address<0>(storage(), 0);
		}

		annotate_nodiscard
		iterator begin(std::size_t capacity)
		{
			return allocator().address(storage(), capacity);
		}

		annotate_nodiscard
		const_iterator begin(std::size_t capacity) const
		{
			return allocator().address(storage(), capacity);
		}

		annotate_nodiscard
		iterator end(std::size_t capacity)
		{
			return begin(capacity) + capacity;
		}

		annotate_nodiscard
		const_iterator end(std::size_t capacity) const
		{
			return begin(capacity) + capacity;
		}

		annotate_nodiscard
		position place(std::size_t index)
		{
			return begin_for(mpl::index_sequence<0>{}) + index;
		}

		annotate_nodiscard
		const_position place(std::size_t index) const
		{
			return begin_for(mpl::index_sequence<0>{}) + index;
		}

		template <typename Position,
		          REQUIRES((std::is_convertible<Position, const_position>::value))>
		annotate_nodiscard
		std::ptrdiff_t index_of(Position pos) const
		{
			return static_cast<Position>(pos) - begin_for(mpl::index_sequence<0>{});
		}

		template <typename Iterator,
		          REQUIRES((!std::is_convertible<Iterator, const_position>::value))>
		annotate_nodiscard
		std::ptrdiff_t index_of(Iterator it) const
		{
			return index_of(utility::select_first<sizeof...(Ts)>(it));
		}

	private:
		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		void * & storage() { return impl_.storage_; }
		const void * storage() const { return impl_.storage_; }
	};

	namespace detail
	{
		template <template <typename> class Allocator, typename T0, typename ...Ts>
		struct unpacked_dynamic_storage_data
		{
			using storage_type = dynamic_storage_impl<Allocator, T0, Ts...>;

			storage_type storage_;
			utility::zip_iterator<Ts *...> extra_begins_;

			T0 * begin(mpl::index_constant<0>)
			{
				return storage_.begin_for(mpl::index_sequence<0>{});
			}

			const T0 * begin(mpl::index_constant<0>) const
			{
				return storage_.begin_for(mpl::index_sequence<0>{});
			}

			template <std::size_t I,
			          REQUIRES((0 < I))>
			mpl::type_at<(I - 1), Ts...> * begin(mpl::index_constant<I>)
			{
				return std::get<(I - 1)>(extra_begins_);
			}

			template <std::size_t I,
			          REQUIRES((0 < I))>
			const mpl::type_at<(I - 1), Ts...> * begin(mpl::index_constant<I>) const
			{
				return std::get<(I - 1)>(extra_begins_);
			}

			void set(utility::zip_iterator<T0 *, Ts *...> it)
			{
				extra_begins_ = utility::last<sizeof...(Ts)>(it);
			}
		};

		template <template <typename> class Allocator, typename T>
		struct unpacked_dynamic_storage_data<Allocator, T>
		{
			using storage_type = dynamic_storage_impl<Allocator, T>;

			storage_type storage_;

			T * begin(mpl::index_constant<0>)
			{
				return storage_.begin_for(mpl::index_sequence<0>{});
			}

			const T * begin(mpl::index_constant<0>) const
			{
				return storage_.begin_for(mpl::index_sequence<0>{});
			}

			void set(utility::zip_iterator<T *> /*it*/)
			{
			}
		};

		template <template <typename> class Allocator, typename ...Ts>
		class unpacked_dynamic_storage_impl
		{
		public:
			using storage_type = typename unpacked_dynamic_storage_data<Allocator, Ts...>::storage_type;

			using pointer = typename storage_type::pointer;
			using const_pointer = typename storage_type::const_pointer;
			using reference = typename storage_type::reference;
			using const_reference = typename storage_type::const_reference;

			template <typename Storing>
			using value_type_for = Storing;
			template <typename T>
			using storing_type_for = T;

			using position = typename storage_type::position;
			using const_position = typename storage_type::const_position;

		private:
			unpacked_dynamic_storage_data<Allocator, Ts...> data_;

		public:
			annotate_nodiscard
			bool allocate(std::size_t capacity)
			{
				const auto ret = data_.storage_.allocate(capacity);
				data_.set(data_.storage_.begin(capacity));
				return ret;
			}

			annotate_nodiscard
			bool good() const
			{
				return data_.storage_.good();
			}

			void deallocate(std::size_t capacity)
			{
				return data_.storage_.deallocate(capacity);
			}

			template <typename T, typename ...Ps>
			T & construct_at(T * ptr_, Ps && ...ps)
			{
				return data_.storage_.construct_at(ptr_, std::forward<Ps>(ps)...);
			}

			template <typename T>
			void destruct_at(T * ptr_)
			{
				data_.storage_.destruct_at(ptr_);
			}

			template <typename T>
			annotate_nodiscard
			T * data(T * ptr_)
			{
				return data_.storage_.data(ptr_);
			}

			template <typename T>
			annotate_nodiscard
			const T * data(const T * ptr_) const
			{
				return data_.storage_.data(ptr_);
			}

			template <typename ...Us,
			          typename Pointer = utility::zip_pointer<Us *...>>
			annotate_nodiscard
			Pointer data(utility::zip_iterator<Us *...> it)
			{
				return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
			}

			template <typename ...Us,
			          typename Pointer = utility::zip_pointer<const Us *...>>
			annotate_nodiscard
			Pointer data(utility::zip_iterator<const Us *...> it) const
			{
				return ext::apply([this](auto * ...ss){ return Pointer(this->data(ss)...); }, it);
			}

			template <typename T>
			annotate_nodiscard
			T * iter(T * ptr)
			{
				return data_.storage_.data(ptr);
			}

			template <typename T>
			annotate_nodiscard
			const T * iter(const T * ptr) const
			{
				return data_.storage_.data(ptr);
			}

			template <typename ...Us,
			          typename Iterator = utility::zip_iterator<Us *...>>
			annotate_nodiscard
			Iterator iter(utility::zip_pointer<Us *...> ptr)
			{
				return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
			}

			template <typename ...Us,
			          typename Iterator = utility::zip_iterator<const Us *...>>
			annotate_nodiscard
			Iterator iter(utility::zip_pointer<const Us *...> ptr) const
			{
				return ext::apply([this](auto * ...ss){ return Iterator(this->iter(ss)...); }, ptr);
			}

			template <std::size_t ...Is,
			          typename Iterator = utility::combine<utility::zip_iterator,
			                                               typename storage_type::template storing_type_for<mpl::type_at<Is, Ts...>> *...>>
			annotate_nodiscard
			Iterator begin_for(mpl::index_sequence<Is...>)
			{
				return Iterator(data_.begin(mpl::index_constant<Is>{})...);
			}

			template <std::size_t ...Is,
			          typename Iterator = utility::combine<utility::zip_iterator,
			                                               const typename storage_type::template storing_type_for<mpl::type_at<Is, Ts...>> *...>>
			annotate_nodiscard
			Iterator begin_for(mpl::index_sequence<Is...>) const
			{
				return Iterator(data_.begin(mpl::index_constant<Is>{})...);
			}

			annotate_nodiscard
			auto begin() { return begin_for(mpl::make_index_sequence_for<Ts...>{}); }
			annotate_nodiscard
			auto begin() const { return begin_for(mpl::make_index_sequence_for<Ts...>{}); }

			annotate_nodiscard
			auto begin(std::size_t /*capacity*/) { return begin(); }
			annotate_nodiscard
			auto begin(std::size_t /*capacity*/) const { return begin(); }

			template <std::size_t ...Is,
			          typename Iterator = utility::combine<utility::zip_iterator,
			                                               typename storage_type::template storing_type_for<mpl::type_at<Is, Ts...>> *...>>
			annotate_nodiscard
			Iterator end_for(std::size_t capacity, mpl::index_sequence<Is...>)
			{
				return Iterator(data_.begin(mpl::index_constant<Is>{}) + capacity...);
			}

			template <std::size_t ...Is,
			          typename Iterator = utility::combine<utility::zip_iterator,
			                                               const typename storage_type::template storing_type_for<mpl::type_at<Is, Ts...>> *...>>
			annotate_nodiscard
			Iterator end_for(std::size_t capacity, mpl::index_sequence<Is...>) const
			{
				return Iterator(data_.begin(mpl::index_constant<Is>{}) + capacity...);
			}

			annotate_nodiscard
			auto end(std::size_t capacity) { return end_for(capacity, mpl::make_index_sequence_for<Ts...>{}); }
			annotate_nodiscard
			auto end(std::size_t capacity) const { return end_for(capacity, mpl::make_index_sequence_for<Ts...>{}); }

			annotate_nodiscard
			position place(std::size_t index)
			{
				return begin_for(mpl::index_sequence<0>{}) + index;
			}

			annotate_nodiscard
			const_position place(std::size_t index) const
			{
				return begin_for(mpl::index_sequence<0>{}) + index;
			}

			template <typename PositionOrIterator>
			annotate_nodiscard
			std::ptrdiff_t index_of(PositionOrIterator pos_or_it) const
			{
				return data_.storage_.index_of(pos_or_it);
			}
		};
	}

	template <typename Storage>
	class basic_storage
		: public Storage
	{
		using base_type = Storage;

		template <typename ...Ss>
		using reference_for = utility::combine<utility::compound,
		                                       typename base_type::template value_type_for<Ss> &...>;
		template <typename ...Ss>
		using rvalue_reference_for = utility::combine<utility::compound,
		                                              typename base_type::template value_type_for<Ss> &&...>;

		template <typename Storing, typename InputIt>
		using can_memcpy = mpl::conjunction<std::is_trivially_copyable<Storing>,
		                                    utility::is_contiguous_iterator<InputIt>>;
		template <typename Storing>
		using can_memset = std::is_trivially_copyable<Storing>;

	public:
		template <typename S, typename ...Ps>
		decltype(auto) construct_at_(S * s, Ps && ...ps) { return this->construct_at(s, std::forward<Ps>(ps)...); }

		template <typename S, typename P>
		decltype(auto) construct_at_(S * s, std::piecewise_construct_t, P && p)
		{
			return ext::apply([&s, this](auto && ...ps) -> decltype(auto) { return construct_at_(s, std::forward<decltype(ps)>(ps)...); }, std::forward<P>(p));
		}

		template <typename ...Ss, typename ...Ps,
		          typename Reference = reference_for<Ss...>,
		          REQUIRES((!ext::is_tuple<Ps...>::value)),
		          REQUIRES((!mpl::is_same<std::piecewise_construct_t, mpl::car<mpl::remove_cvref_t<Ps>..., void>>::value))>
		Reference construct_at_(utility::zip_iterator<Ss *...> it, Ps && ...ps)
		{
			return ext::apply([&ps..., this](auto * ...ss) -> Reference { return Reference(construct_at_(ss, std::forward<Ps>(ps))...); }, it);
		}

		template <typename ...Ss, typename P,
		          REQUIRES((ext::tuple_size<P>::value == sizeof...(Ss)))>
		decltype(auto) construct_at_(utility::zip_iterator<Ss *...> it, P && p)
		{
			return ext::apply([&it, this](auto && ...ps) -> decltype(auto) { return construct_at_(it, std::forward<decltype(ps)>(ps)...); }, std::forward<P>(p));
		}

		template <typename ...Ss, typename ...Ps,
		          typename Reference = reference_for<Ss...>>
		Reference construct_at_(utility::zip_iterator<Ss *...> it, std::piecewise_construct_t, Ps && ...ps)
		{
			return ext::apply([&ps..., this](auto * ...ss) -> Reference { return Reference(construct_at_(ss, std::piecewise_construct, std::forward<Ps>(ps))...); }, it);
		}

		template <typename S, typename ...Ps>
		S * construct_fill(S * begin, ext::usize count, Ps && ...ps)
		{
			for (const S * const end = begin + count; begin != end; begin++)
			{
				construct_at_(begin, ps...);
			}
			return begin;
		}

		template <typename S>
		S * construct_fill(S * begin, ext::usize count, utility::zero_initialize_t)
		{
			return construct_fill_zero_initialize_impl(can_memset<S>{}, begin, count);
		}

		template <typename ...Ss>
		auto construct_fill(utility::zip_iterator<Ss *...> it, ext::usize count)
		{
			return ext::apply([this, count](auto * ...ss){ return utility::zip_iterator<Ss *...>(this->construct_fill(ss, count)...); }, it);
		}

		template <typename ...Ss, typename ...Ps,
		          REQUIRES((0 < sizeof...(Ps))),
		          REQUIRES((!mpl::is_same<utility::zero_initialize_t, mpl::car<mpl::remove_cvref_t<Ps>..., void>>::value))>
		auto construct_fill(utility::zip_iterator<Ss *...> it, ext::usize count, Ps && ...ps)
		{
			return ext::apply([&ps..., this, count](auto * ...ss){ return utility::zip_iterator<Ss *...>(this->construct_fill(ss, count, ps)...); }, it);
		}

		template <typename ...Ss>
		auto construct_fill(utility::zip_iterator<Ss *...> it, ext::usize count, utility::zero_initialize_t)
		{
			return ext::apply([this, count](auto * ...ss){ return utility::zip_iterator<Ss *...>(this->construct_fill(ss, count, utility::zero_initialize)...); }, it);
		}

		template <typename ...Ss, typename ...Ps>
		auto construct_fill(utility::zip_iterator<Ss *...> it, ext::usize count, std::piecewise_construct_t, Ps && ...ps)
		{
			return ext::apply([&ps..., this, count](auto * ...ss){ return utility::zip_iterator<Ss *...>(this->construct_fill(ss, count, std::piecewise_construct, std::forward<Ps>(ps))...); }, it);
		}

		template <typename S,
		          REQUIRES((can_memset<S>::value))>
		S * memset_fill(S * start, ext::usize count, ext::byte value)
		{
			std::memset(this->data(start), static_cast<int>(value), count * sizeof(S));

			return start + count;
		}

		template <typename ...Ss,
		          REQUIRES((mpl::conjunction<can_memset<Ss>...>::value))>
		auto memset_fill(utility::zip_iterator<Ss *...> it, ext::usize count, ext::byte value)
		{
			return ext::apply([this, count, value](auto * ...ss){ return utility::zip_iterator<Ss *...>(this->memset_fill(ss, count, value)...); }, it);
		}

		template <typename S, typename InputIt>
		S * construct_range(S * start, InputIt begin, InputIt end)
		{
			return construct_range_or_memcpy_impl(can_memcpy<S, InputIt>{}, start, begin, end);
		}

		template <typename ...Ss, typename InputIt>
		auto construct_range(utility::zip_iterator<Ss *...> it, InputIt begin, InputIt end)
		{
			return construct_range_impl(mpl::make_index_sequence_for<Ss...>{}, it, begin, end, 0);
		}

		template <typename S, typename InputIt,
		          REQUIRES((can_memcpy<S, InputIt>::value))>
		S * memcpy_range(S * start, InputIt begin, InputIt end)
		{
			return construct_range_or_memcpy_impl(mpl::true_type{}, start, begin, end);
		}

		template <typename ...Ss, typename InputIt,
		          REQUIRES((mpl::conjunction<can_memcpy<Ss, ext::tuple_element<mpl::index_of<Ss, Ss...>::value, InputIt>>...>::value))>
		auto memcpy_range(utility::zip_iterator<Ss *...> it, InputIt begin, InputIt end)
		{
			return memcpy_range_impl(mpl::make_index_sequence_for<Ss...>{}, it, begin, end);
		}

		using base_type::destruct_at;

		template <typename ...Ss>
		void destruct_at(utility::zip_iterator<Ss *...> it)
		{
			utl::for_each(it, [this](auto * s){ destruct_at(s); });
		}

		template <typename S>
		void destruct_range(S * begin, S * end)
		{
			for (; begin != end; begin++)
			{
				destruct_at(begin);
			}
		}

		template <typename ...Ss>
		void destruct_range(utility::zip_iterator<Ss *...> begin, utility::zip_iterator<Ss *...> end)
		{
			destruct_range_impl(mpl::make_index_sequence_for<Ss...>{}, begin, end);
		}

	private:
		template <typename S>
		S * construct_fill_zero_initialize_impl(mpl::true_type /*can_memset*/, S * begin, ext::usize count)
		{
			return memset_fill(begin, count, ext::byte{});
		}

		template <typename S>
		S * construct_fill_zero_initialize_impl(mpl::false_type /*can_memset*/, S * begin, ext::usize count)
		{
			// todo forward initialization strategy
			return construct_fill(begin, count);
		}

		template <typename S, typename InputIt>
		S * construct_range_or_memcpy_impl(mpl::true_type /*can_memcpy*/, S * start, InputIt begin, InputIt end)
		{
			static_assert(mpl::is_same<typename base_type::template value_type_for<S>, typename std::iterator_traits<InputIt>::value_type>::value, "Memcpying between different types seem like an error! Are you doing something you should not?");

			using utility::raw_range;
			auto range = raw_range(begin, end);

			assert(range.first <= range.second);
			std::memcpy(this->data(start), range.first, (range.second - range.first) * sizeof(S));

			return start + (range.second - range.first);
		}

		template <typename S, typename InputIt>
		S * construct_range_or_memcpy_impl(mpl::false_type /*can_memcpy*/, S * start, InputIt begin, InputIt end)
		{
			for (; begin != end; start++, ++begin)
			{
				construct_at_(start, *begin);
			}
			return start;
		}

		template <std::size_t ...Is, typename ...Ss, typename InputIt>
		auto construct_range_impl(mpl::index_sequence<Is...>, utility::zip_iterator<Ss *...> it, InputIt begin, InputIt end, ...)
		{
			return utility::zip_iterator<Ss *...>(construct_range(std::get<Is>(it), std::get<Is>(begin), std::get<Is>(end))...);
		}

		// todo weird
		template <std::size_t ...Is, typename ...Ss, typename InputIt>
		auto construct_range_impl(mpl::index_sequence<Is...>, utility::zip_iterator<Ss *...> it, std::move_iterator<InputIt> begin, std::move_iterator<InputIt> end, int)
		{
			return utility::zip_iterator<Ss *...>(construct_range(std::get<Is>(it), std::make_move_iterator(std::get<Is>(begin.base())), std::make_move_iterator(std::get<Is>(end.base())))...);
		}

		template <std::size_t ...Is, typename ...Ss, typename InputIt>
		auto memcpy_range(mpl::index_sequence<Is...>, utility::zip_iterator<Ss *...> it, InputIt begin, InputIt end)
		{
			return utility::zip_iterator<Ss *...>(construct_range_or_memcpy_impl(mpl::true_type{}, std::get<Is>(it), std::get<Is>(begin), std::get<Is>(end))...);
		}

		template <std::size_t ...Is, typename ...Ss>
		void destruct_range_impl(mpl::index_sequence<Is...>, utility::zip_iterator<Ss *...> begin, utility::zip_iterator<Ss *...> end)
		{
			int expansion_hack[] = {(destruct_range(std::get<Is>(begin), std::get<Is>(end)), 0)...};
			static_cast<void>(expansion_hack);
		}
	};

	template <std::size_t Capacity, typename ...Ts>
	using static_storage = basic_storage<static_storage_impl<Capacity, Ts...>>;
	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_storage = basic_storage<dynamic_storage_impl<Allocator, Ts...>>;

	template <typename ...Ts>
	using heap_storage = dynamic_storage<utility::heap_allocator, Ts...>;

	template <typename Storage>
	struct storage_traits;
	template <template <typename> class Allocator, typename ...Ts>
	struct storage_traits<dynamic_storage<Allocator, Ts...>>
	{
		template <typename T>
		using allocator_type = Allocator<T>;
		using type_list = mpl::type_list<Ts...>; // todo remove, and implement storage_element instead

		template <typename ...Us>
		using storage_type = dynamic_storage<Allocator, Us...>;
		template <typename ...Us>
		using append = dynamic_storage<Allocator, Ts..., Us...>; // todo remove

		using unpacked = basic_storage<detail::unpacked_dynamic_storage_impl<Allocator, Ts...>>; // todo remove

		using static_capacity = mpl::false_type;
		using trivial_allocate = mpl::false_type;
		using trivial_deallocate = mpl::false_type;
		using moves_allocation = mpl::true_type;

		static constexpr std::size_t size = sizeof...(Ts); // todo remove
	};
	template <std::size_t Capacity, typename ...Ts>
	struct storage_traits<static_storage<Capacity, Ts...>>
	{
		template <typename T>
		using allocator_type = utility::null_allocator<T>;
		using type_list = mpl::type_list<Ts...>; // todo remove, and implement storage_element instead

		template <typename ...Us>
		using storage_type = static_storage<Capacity, Us...>;
		template <typename ...Us>
		using append = static_storage<Capacity, Ts..., Us...>; // todo remove

		using unpacked = static_storage<Capacity, Ts...>; // todo remove

		using static_capacity = mpl::true_type;
		using trivial_allocate = mpl::true_type;
		using trivial_deallocate = mpl::true_type;
		using moves_allocation = mpl::false_type;

		static constexpr std::size_t size = sizeof...(Ts); // todo remove
		static constexpr std::size_t capacity_value = Capacity;
	};

	template <typename Storage>
	using storage_size = mpl::index_constant<storage_traits<Storage>::size>;

	template <std::size_t Capacity>
	using static_storage_traits = storage_traits<static_storage<Capacity, int>>; // todo remove int?
	template <template <typename> class Allocator>
	using dynamic_storage_traits = storage_traits<dynamic_storage<Allocator, int>>; // todo remove int?
	using heap_storage_traits = dynamic_storage_traits<heap_allocator>;
	using null_storage_traits = dynamic_storage_traits<null_allocator>;

	template <typename Storage>
	using storage_is_trivially_destructible =
		mpl::conjunction<typename Storage::storing_trivially_destructible,
		                 typename storage_traits<Storage>::trivial_deallocate>;
	template <typename Storage>
	using storage_is_trivially_default_constructible =
		mpl::conjunction<typename Storage::storing_trivially_default_constructible,
		                 typename storage_traits<Storage>::trivial_allocate>;

	template <typename Storage>
	struct reserve_exact
	{
		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t /*size*/)
		{
			return StorageTraits::capacity_value;
		}

		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((!StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t size)
		{
			return size;
		}
	};

	template <typename Storage>
	struct reserve_power_of_two
	{
		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t /*size*/)
		{
			return StorageTraits::capacity_value;
		}

		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((!StorageTraits::static_capacity::value))>
		/*constexpr*/ std::size_t operator () (std::size_t size)
		{
			return utility::clp2(size);
		}
	};

	template <std::size_t Size, template <typename> class ReservationStrategy>
	struct reserve_at_least
	{
		template <typename Storage>
		struct type
		{
			constexpr std::size_t operator () (std::size_t size)
			{
				return ReservationStrategy<Storage>{}(size < Size ? Size : size);
			}
		};
	};

	template <template <typename> class ReservationStrategy>
	using reserve_nonempty = reserve_at_least<1, ReservationStrategy>;
}
