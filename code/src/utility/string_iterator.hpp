#pragma once

#include "utility/encoding_traits.hpp"
#include "utility/type_traits.hpp"

#include <utility>

namespace utility
{
	template <typename Encoding, bool Const>
	class string_iterator_impl
	{
		template <typename Encoding_, bool Const_>
		friend class string_iterator_impl;

		using this_type = string_iterator_impl<Encoding, Const>;

		using encoding_traits = encoding_traits<Encoding>;

	public:
		using difference_type = utility::lazy_difference<Encoding>; // todo only if code unit != code point
		using value_type = typename encoding_traits::code_unit;
		using pointer = mpl::add_const_if<Const, value_type> *;
		using reference = typename encoding_traits::code_point; // todo typename Encoding::references
		using iterator_category = std::random_access_iterator_tag;
		using size_type = std::size_t;
	private:
		using code_unit = typename encoding_traits::code_unit;
		using code_point = typename encoding_traits::code_point;

	private:
		pointer ptr_;

	public:
		string_iterator_impl() = default;
		constexpr string_iterator_impl(pointer ptr) : ptr_(ptr) {}
		template <bool ConstOther,
		          REQUIRES((std::is_constructible<pointer,
		                    typename string_iterator_impl<Encoding, ConstOther>::pointer>::value))>
		string_iterator_impl(string_iterator_impl<Encoding, ConstOther> other) :
			ptr_(other.ptr_)
		{}

	public:
		constexpr reference operator * () const { return encoding_traits::dereference(ptr_); }

		constexpr this_type & operator ++ () { ptr_ += encoding_traits::next(ptr_); return *this; }
		constexpr this_type operator ++ (int) { auto tmp = *this; ptr_ += encoding_traits::next(ptr_); return tmp; }
		constexpr this_type & operator -- () { ptr_ -= encoding_traits::previous(ptr_); return *this; }
		constexpr this_type operator -- (int) { auto tmp = *this; ptr_ -= encoding_traits::previous(ptr_); return tmp; }

		template <typename Difference>
		constexpr this_type & operator += (Difference && difference)
		{
			ptr_ += encoding_traits::next(ptr_, std::forward<Difference>(difference));
			return *this;
		}
		template <typename Difference>
		constexpr this_type & operator -= (Difference && difference)
		{
			ptr_ -= encoding_traits::previous(ptr_, std::forward<Difference>(difference));
			return *this;
		}

		template <typename Difference>
		constexpr reference operator [] (Difference && difference) const
		{
			return encoding_traits::dereference(ptr_ + encoding_traits::next(ptr_, std::forward<Difference>(difference)));
		}

		constexpr pointer get() const { return ptr_; }

		template <typename Difference>
		friend constexpr this_type operator + (this_type x, Difference && difference)
		{
			return x += std::forward<Difference>(difference);
		}
		template <typename Difference>
		friend constexpr this_type operator + (Difference && difference, this_type x)
		{
			return x += std::forward<Difference>(difference);
		}
		template <typename Difference>
		friend constexpr this_type operator - (this_type x, Difference && difference)
		{
			return x -= std::forward<Difference>(difference);
		}

		friend constexpr bool operator == (this_type x, this_type y) { return x.ptr_ == y.ptr_; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return x.ptr_ < y.ptr_; }
		friend constexpr bool operator <= (this_type x, this_type y) { return !(y < x); }
		friend constexpr bool operator > (this_type x, this_type y) { return y < x; }
		friend constexpr bool operator >= (this_type x, this_type y) { return !(x < y); }

		friend constexpr difference_type operator - (this_type x, this_type y) { return {y.ptr_, x.ptr_}; }
	};

	template <typename Encoding>
	using string_iterator = string_iterator_impl<Encoding, false>;
	template <typename Encoding>
	using const_string_iterator = string_iterator_impl<Encoding, true>;
}
