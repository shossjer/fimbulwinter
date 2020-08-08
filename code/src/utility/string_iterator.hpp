#pragma once

#include "utility/concepts.hpp"
#include "utility/encoding_traits.hpp"
#include "utility/type_traits.hpp"

#include <utility>

namespace utility
{
	template <typename Boundary, bool Const>
	class string_iterator_impl
	{
		template <typename Boundary_, bool Const_>
		friend class string_iterator_impl;

		using this_type = string_iterator_impl<Boundary, Const>;

	public:

		using value_type = typename Boundary::value_type;
		using pointer = mpl::conditional_t<Const,
		                                   typename Boundary::const_pointer,
		                                   typename Boundary::pointer>;
		using reference = mpl::conditional_t<Const,
		                                     typename Boundary::const_reference,
		                                     typename Boundary::reference>;
		using difference_type = typename Boundary::difference_type;
		using iterator_category = std::random_access_iterator_tag; // todo iff unit boundary, else bidirectional

	private:

		pointer ptr_;

	public:

		string_iterator_impl() = default;

		explicit constexpr string_iterator_impl(pointer ptr) : ptr_(ptr) {}

		template <bool ConstOther,
		          REQUIRES((std::is_constructible<
			                    pointer,
			                    typename string_iterator_impl<Boundary, ConstOther>::pointer>::value))>
		explicit constexpr string_iterator_impl(string_iterator_impl<Boundary, ConstOther> other)
			: ptr_(other.ptr_)
		{}

	public:

		constexpr reference operator * () const { return Boundary::dereference(ptr_); }

		constexpr this_type & operator ++ () { ptr_ += Boundary::next(ptr_); return *this; }
		constexpr this_type operator ++ (int) { auto tmp = *this; ptr_ += Boundary::next(ptr_); return tmp; }
		constexpr this_type & operator -- () { ptr_ -= Boundary::previous(ptr_); return *this; }
		constexpr this_type operator -- (int) { auto tmp = *this; ptr_ -= Boundary::previous(ptr_); return tmp; }

		constexpr this_type & operator += (ext::ssize count)
		{
			ptr_ += Boundary::next(ptr_, count);
			return *this;
		}

		constexpr this_type & operator -= (ext::ssize count)
		{
			ptr_ -= Boundary::previous(ptr_, count);
			return *this;
		}

		constexpr reference operator [] (ext::ssize count) const
		{
			return Boundary::dereference(ptr_ + Boundary::next(ptr_, count));
		}

		constexpr pointer get() const { return ptr_; }

		friend constexpr this_type operator + (this_type x, ext::ssize count) { return x += count; }
		friend constexpr this_type operator + (ext::ssize count, this_type x) { return x += count; }
		friend constexpr this_type operator - (this_type x, ext::ssize count) { return x -= count; }

		friend constexpr bool operator == (this_type x, this_type y) { return x.ptr_ == y.ptr_; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return x.ptr_ < y.ptr_; }
		friend constexpr bool operator <= (this_type x, this_type y) { return !(y < x); }
		friend constexpr bool operator > (this_type x, this_type y) { return y < x; }
		friend constexpr bool operator >= (this_type x, this_type y) { return !(x < y); }

		friend constexpr difference_type operator - (this_type x, this_type y) { return x.ptr_ - y.ptr_; }
	};

	template <typename Boundary>
	using string_iterator = string_iterator_impl<Boundary, false>;
	template <typename Boundary>
	using const_string_iterator = string_iterator_impl<Boundary, true>;
}
