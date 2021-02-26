#pragma once

#include "utility/ext/stddef.hpp"
#include "utility/type_traits.hpp"

#include <iterator>

namespace ext
{
	template <typename T, typename Iter>
	class cast_iterator
	{
		using this_type = cast_iterator<T, Iter>;

	public:

		using value_type = mpl::remove_cvref_t<T>;
		using pointer = mpl::remove_reference_t<T> *;
		using reference = T;
		using difference_type = typename std::iterator_traits<Iter>::difference_type;
		using iterator_category = typename std::iterator_traits<Iter>::iterator_category;

	private:

		Iter iter_;

	public:

		cast_iterator() = default;

		explicit constexpr cast_iterator(Iter iter) : iter_(static_cast<Iter &&>(iter)) {}

	public:

		constexpr reference operator * () const { return static_cast<reference>(*iter_); }

		constexpr this_type & operator ++ () { ++iter_; return *this; }
		constexpr this_type operator ++ (int) { auto tmp = *this; ++iter_; return tmp; }
		constexpr this_type & operator -- () { --iter_; return *this; }
		constexpr this_type operator -- (int) { auto tmp = *this; --iter_; return tmp; }

		constexpr this_type & operator += (ext::ssize count) { iter_ += count; return *this; }
		constexpr this_type & operator -= (ext::ssize count) { iter_ -= count; return *this; }

		constexpr reference operator [] (ext::ssize count) const { return static_cast<reference>(iter_[count]); }

		friend constexpr this_type operator + (this_type x, ext::ssize count) { return x += count; }
		friend constexpr this_type operator + (ext::ssize count, this_type x) { return x += count; }
		friend constexpr this_type operator - (this_type x, ext::ssize count) { return x -= count; }

		friend constexpr bool operator == (this_type x, this_type y) { return x.iter_ == y.iter_; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return x.iter_ < y.iter_; }
		friend constexpr bool operator <= (this_type x, this_type y) { return !(y < x); }
		friend constexpr bool operator > (this_type x, this_type y) { return y < x; }
		friend constexpr bool operator >= (this_type x, this_type y) { return !(x < y); }

		friend constexpr difference_type operator - (this_type x, this_type y) { return x.iter_ - y.iter_; }
	};

	template <typename T, typename Iter>
	constexpr cast_iterator<T, mpl::remove_cvref_t<Iter>> make_cast_iterator(Iter && iter)
	{
		return cast_iterator<T, mpl::remove_cvref_t<Iter>>(static_cast<Iter &&>(iter));
	}
}
