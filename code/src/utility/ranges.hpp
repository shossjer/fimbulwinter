#pragma once

#include <cstdint>

namespace ranges
{
	template <typename T>
	class iota_iterator
	{
		using this_type = iota_iterator<T>;

	public:
		using value_type = T;
		using reference = T;

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
	};

	inline constexpr auto index_sequence(std::size_t from, std::size_t to)
	{
		return iota_range<std::size_t>(from, to);
	}

	inline constexpr auto index_sequence(std::size_t count)
	{
		return index_sequence(0, count);
	}

	template <typename T>
	constexpr auto index_sequence_for(const T & container)
		-> decltype(index_sequence(container.size()))
	{
		return index_sequence(container.size());
	}
}
