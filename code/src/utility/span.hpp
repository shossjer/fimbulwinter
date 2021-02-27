#pragma once

#include "utility/concepts.hpp"
#include "utility/iterator.hpp"
#include "utility/ext/stddef.hpp"

namespace mpl
{
	template <typename From, typename To>
	using is_array_convertible = std::is_convertible<From (*)[], To (*)[]>;
}

namespace utility
{
	/*inline */constexpr std::size_t dynamic_extent = std::size_t(-1);

	namespace detail
	{
		template <typename T, std::size_t Extent>
		class span_impl
		{
		protected:
			T * ptr_;

		public:
			template <std::size_t Extent_ = Extent,
			          REQUIRES((Extent_ == 0))>
			constexpr span_impl()
				: ptr_(nullptr)
			{}

			constexpr span_impl(T * ptr, std::size_t /*size*/)
				: ptr_(ptr)
			{}

		public:
			constexpr std::size_t size() const { return Extent; }
		};

		template <typename T>
		class span_impl<T, dynamic_extent>
		{
		protected:
			T * ptr_;
			std::size_t size_;

		public:
			constexpr span_impl()
				: ptr_(nullptr)
				, size_(0)
			{}

			constexpr span_impl(T * ptr, std::size_t size)
				: ptr_(ptr)
				, size_(size)
			{}

		public:
			std::size_t size() const { return size_; }
		};
	}

	template <typename T, std::size_t Extent = dynamic_extent>
	class span
		: public detail::span_impl<T, Extent>
	{
		using this_type = span<T, Extent>;
		using base_type = detail::span_impl<T, Extent>;

	public:
		using value_type = mpl::remove_cv_t<T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T *;
		using const_pointer = const T *;
		using reference = T &;
		using const_reference = const T &;
		using iterator = pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;

	public:
		constexpr span() = default;

		constexpr span(const this_type &) = default;

		template <typename It,
		          REQUIRES((ext::is_contiguous_iterator<It>::value)),
		          REQUIRES((mpl::is_same<T,
		                                 mpl::add_const_if<std::is_const<T>::value,
		                                                   mpl::remove_reference_t<decltype(*std::declval<It>())>>>::value))>
		constexpr span(It first, size_type count)
			: base_type(utility::to_address(first), count)
		{}

		template <typename It,
		          REQUIRES((ext::is_contiguous_iterator<It>::value)),
		          REQUIRES((mpl::is_same<T,
		                                 mpl::add_const_if<std::is_const<T>::value,
		                                                   mpl::remove_reference_t<decltype(*std::declval<It>())>>>::value))>
		constexpr span(It first, It end)
			: base_type(utility::to_address(first), end - first)
		{}

		template <std::size_t N,
		          REQUIRES((N == Extent || Extent == dynamic_extent))>
		constexpr span(T (& arr)[N])
			: base_type(arr, N)
		{}

		template <typename U, std::size_t N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<U, T>::value))>
		constexpr span(std::array<U, N> & arr)
			: base_type(arr.data(), N)
		{}

		template <typename U, std::size_t N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<const U, T>::value))>
		constexpr span(const std::array<U, N> & arr)
			: base_type(arr.data(), N)
		{}

		template <typename U, std::size_t N,
		          REQUIRES((N == Extent || Extent == dynamic_extent)),
		          REQUIRES((mpl::is_array_convertible<U, T>::value))>
		constexpr span(const span<U, N> & other)
			: base_type(other.data(), other.size())
		{}

	public:
		constexpr iterator begin() const { return this->ptr_; }
		constexpr iterator end() const { return this->ptr_ + this->size(); }
		/*constexpr */reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		/*constexpr */reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }

		constexpr reference operator [] (ext::index i) const { return this->ptr_[i]; }
		constexpr pointer data() const { return this->ptr_; }
	};

	template <typename T, std::size_t N>
	constexpr typename span<T, N>::reference front(const span<T, N> & s) { return s[0]; }

	template <typename T, std::size_t N>
	constexpr typename span<T, N>::reference back(const span<T, N> & s) { return s[s.size() - 1]; }

	template <typename T, std::size_t N>
	constexpr bool empty(const span<T, N> & s) { return s.size() == 0; }

	template <std::size_t Count,
	          typename T, std::size_t N,
	          REQUIRES((Count <= N))>
	constexpr span<T, Count> first(const span<T, N> & s) { return {s.data(), Count}; }

	template <typename T, std::size_t N>
	constexpr span<T, dynamic_extent> first(const span<T, N> & s, std::size_t count) { return {s.data(), count}; }

	template <std::size_t Count,
	          typename T, std::size_t N,
	          REQUIRES((Count <= N))>
	constexpr span<T, Count> last(const span<T, N> & s) { return {s.data() + (s.size() - Count), Count}; }

	template <typename T, std::size_t N>
	constexpr span<T, dynamic_extent> last(const span<T, N> & s, std::size_t count) { return {s.data() + (s.size() - count), count}; }

	template <std::size_t Offset, std::size_t Count = dynamic_extent,
	          typename T, std::size_t N,
	          REQUIRES((Offset <= N)),
	          REQUIRES((Count == dynamic_extent || Count <= N - Offset))>
	constexpr span<T, Count != dynamic_extent ? Count : N != dynamic_extent ? N - Offset : dynamic_extent> subspan(const span<T, N> & s) { return {s.data() + Offset, Count == dynamic_extent ? s.size() - Offset : Count}; }

	template <typename T, std::size_t N>
	constexpr span<T, dynamic_extent> subspan(const span<T, N> & s, std::size_t offset, std::size_t count = dynamic_extent) { return {s.data() + offset, count == dynamic_extent ? s.size() - offset : count}; }
}
