#pragma once

#include "utility/concepts.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/iterator.hpp"

namespace mpl
{
	template <typename From, typename To>
	using is_array_convertible = std::is_convertible<From (*)[], To (*)[]>;
}

namespace utility
{
	/*inline */constexpr const ext::usize dynamic_extent = ext::usize(-1);

	namespace detail
	{
		template <typename T, ext::usize Extent>
		class span_impl
		{
		protected:

			T * ptr_;

		public:

			span_impl() = default;

			constexpr explicit span_impl(T * ptr, ext::usize /*size*/)
				: ptr_(ptr)
			{}

		public:

			constexpr ext::usize size() const { return Extent; }

		};

		template <typename T>
		class span_impl<T, dynamic_extent>
		{
		protected:

			T * ptr_;
			ext::usize size_;

		public:

			span_impl() = default;

			constexpr explicit span_impl(T * ptr, ext::usize size)
				: ptr_(ptr)
				, size_(size)
			{}

		public:

			ext::usize size() const { return size_; }

		};
	}

	template <typename T, ext::usize Extent = dynamic_extent>
	class span
		: public detail::span_impl<T, Extent>
	{
		using this_type = span<T, Extent>;
		using base_type = detail::span_impl<T, Extent>;

	public:

		using value_type = mpl::remove_cv_t<T>;
		using size_type = ext::usize;
		using difference_type = std::ptrdiff_t;
		using pointer = T *;
		using const_pointer = const T *;
		using reference = T &;
		using const_reference = const T &;
		using iterator = pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;

	public:

		span() = default;

		span(const this_type &) = default;

		template <typename It,
		          REQUIRES((ext::is_contiguous_iterator<It>::value)),
		          REQUIRES((mpl::is_same<T,
		                                 mpl::add_const_if<std::is_const<T>::value,
		                                                   mpl::remove_reference_t<decltype(*std::declval<It>())>>>::value))>
		constexpr explicit span(It first, size_type count)
			: base_type(utility::to_address(first), count)
		{}

		template <typename It,
		          REQUIRES((ext::is_contiguous_iterator<It>::value)),
		          REQUIRES((mpl::is_same<T,
		                                 mpl::add_const_if<std::is_const<T>::value,
		                                                   mpl::remove_reference_t<decltype(*std::declval<It>())>>>::value))>
		constexpr explicit span(It first, It end)
			: base_type(utility::to_address(first), end - first)
		{}

		template <ext::usize N,
		          REQUIRES((N == Extent || Extent == dynamic_extent))>
		constexpr explicit span(T (& arr)[N])
			: base_type(arr, N)
		{}

		template <typename U, ext::usize N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<U, T>::value))>
		constexpr explicit span(std::array<U, N> & arr)
			: base_type(arr.data(), N)
		{}

		template <typename U, ext::usize N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<const U, T>::value))>
		constexpr explicit span(const std::array<U, N> & arr)
			: base_type(arr.data(), N)
		{}

		template <typename U, ext::usize N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<U, T>::value))>
		constexpr explicit span(const span<U, N> & other)
			: base_type(other.data(), other.size())
		{}

	public:

		constexpr iterator begin() const { return this->ptr_; }
		constexpr iterator end() const { return this->ptr_ + this->size(); }

		constexpr pointer data() const { return this->ptr_; }

		constexpr reference operator [] (ext::index i) const { return this->ptr_[i]; }

	};

	template <typename T, ext::usize N>
	constexpr typename span<T, N>::iterator begin(const span<T, N> & s) { return s.begin(); }
	template <typename T, ext::usize N>
	constexpr typename span<T, N>::iterator end(const span<T, N> & s) { return s.end(); }

	template <typename T, ext::usize N>
	constexpr typename span<T, N>::pointer data(const span<T, N> & s) { return s.data(); }
	template <typename T, ext::usize N>
	constexpr typename span<T, N>::size_type size(const span<T, N> & s) { return s.size(); }

	template <typename T, ext::usize N>
	constexpr bool empty(const span<T, N> & s) { return s.size() == 0; }

	template <typename T, ext::usize N>
	constexpr typename span<T, N>::reference front(const span<T, N> & s) { return s[0]; }

	template <typename T, ext::usize N>
	constexpr typename span<T, N>::reference back(const span<T, N> & s) { return s[s.size() - 1]; }

	template <ext::usize Count,
	          typename T, ext::usize N,
	          REQUIRES((Count <= N))>
	constexpr span<T, Count> first(const span<T, N> & s) { return span<T, Count>(s.data(), Count); }

	template <typename T, ext::usize N>
	constexpr span<T, dynamic_extent> first(const span<T, N> & s, ext::usize count) { return span<T, dynamic_extent>(s.data(), count); }

	template <ext::usize Count,
	          typename T, ext::usize N,
	          REQUIRES((Count <= N))>
	constexpr span<T, Count> last(const span<T, N> & s) { return span<T, Count>(s.data() + (s.size() - Count), Count); }

	template <typename T, ext::usize N>
	constexpr span<T, dynamic_extent> last(const span<T, N> & s, ext::usize count) { return span<T, dynamic_extent>(s.data() + (s.size() - count), count); }

	template <ext::usize Offset, ext::usize Count = dynamic_extent,
	          typename T, ext::usize N,
	          REQUIRES((Offset <= N)),
	          REQUIRES((Count == dynamic_extent || Count <= N - Offset)),
	          ext::usize K = Count != dynamic_extent ? Count : N != dynamic_extent ? N - Offset : dynamic_extent>
	constexpr span<T, K> subspan(const span<T, N> & s) { return span<T, K>(s.data() + Offset, Count == dynamic_extent ? s.size() - Offset : Count); }

	template <typename T, ext::usize N>
	constexpr span<T, dynamic_extent> subspan(const span<T, N> & s, ext::usize offset, ext::usize count = dynamic_extent) { return span<T, dynamic_extent>(s.data() + offset, count == dynamic_extent ? s.size() - offset : count); }

	template <typename T, ext::usize N>
	constexpr span<T, dynamic_extent> drop(const span<T, N> & s, ext::usize count) { return span<T, dynamic_extent>(s.data() + count, s.size() - count); }
}
