#pragma once

#include "utility/compiler.hpp"

#include <cstdint>
#include <iterator>
#include <type_traits>

namespace ranges
{
	template <typename T>
	class iota_iterator
	{
		using this_type = iota_iterator<T>;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = void; // ?
		using reference = T;
		using iterator_category = std::bidirectional_iterator_tag;

	private:
		T value_;

	public:
		explicit constexpr iota_iterator(T value)
			: value_(value)
		{}

	public:
		constexpr reference operator * () const { return value_; }
		constexpr this_type & operator ++ () { ++value_; return *this; }
		constexpr this_type operator ++ (int) { return this_type(value_++); }
		constexpr this_type& operator -- () { --value_; return *this; }
		constexpr this_type operator -- (int) { return this_type(value_--); }

	private:
		friend constexpr bool operator == (this_type a, this_type b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (this_type a, this_type b) { return !(a == b); }
	};

	template <typename T>
	class iota_range
	{
	public:
		using value_type = T;
		using iterator = iota_iterator<T>;

	private:
		T begin_;
		T end_;

	public:
		constexpr iota_range(T begin, T end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr iterator begin() const { return iterator(begin_); }
		constexpr iterator end() const { return iterator(end_); }
		/*constexpr */auto rbegin() const { return std::make_reverse_iterator(end()); }
		/*constexpr */auto rend() const { return std::make_reverse_iterator(begin()); }
	};

	inline constexpr auto index_sequence(std::ptrdiff_t from, std::ptrdiff_t to)
	{
		fiw_assert(from <= to);
		fiw_assert(0 <= from); // more weird than harmful

		return iota_range<std::ptrdiff_t>(from, to);
	}

	template <typename N>
	inline constexpr auto index_sequence(N count)
	{
		using I = std::make_signed_t<N>;

		fiw_assert(0 <= static_cast<I>(count));

		return iota_range<I>(0, static_cast<I>(count));
	}

	template <std::size_t N, typename T>
	constexpr auto index_sequence_for(const T(&)[N]) { return index_sequence(N); }

	template <typename T>
	constexpr auto index_sequence_for(const T & container)
		-> decltype(index_sequence(container.size()))
	{
		return index_sequence(container.size());
	}
}
