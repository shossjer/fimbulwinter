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
		constexpr string_iterator_impl(string_iterator_impl<Boundary, ConstOther> other)
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

	template <typename Boundary>
	constexpr int
	compare(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		typename Boundary::value_type c)
	{
		return ext::strcmp(begin.get(), end.get(), c);
	}

	template <typename Boundary>
	constexpr int
	compare(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		typename Boundary::const_pointer str)
	{
		return ext::strcmp(begin.get(), end.get(), str);
	}

	template <typename Boundary>
	constexpr int
	compare(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		const_string_iterator<Boundary> strbegin,
		const_string_iterator<Boundary> strend)
	{
		return ext::strcmp(begin.get(), end.get(), strbegin.get(), strend.get());
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	find(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		typename Boundary::const_reference c)
	{
		return ext::strfind(begin, end, c);
	}

	template <typename Encoding, bool Const>
	constexpr string_iterator_impl<boundary_unit<Encoding>, Const>
	find(
		string_iterator_impl<boundary_unit<Encoding>, Const> begin,
		string_iterator_impl<boundary_unit<Encoding>, Const> end,
		typename boundary_unit<Encoding>::const_pointer expr)
	{
		return ext::strfind(begin, end, expr, ext::strend(expr));
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	find(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		const_string_iterator<Boundary> exprbegin,
		const_string_iterator<Boundary> exprend)
	{
		return ext::strfind(begin, end, exprbegin.get(), exprend.get());
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	rfind(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		typename Boundary::const_reference c)
	{
		return ext::strrfind(begin, end, c);
	}

	template <typename Encoding, bool Const>
	constexpr string_iterator_impl<boundary_unit<Encoding>, Const>
	rfind(
		string_iterator_impl<boundary_unit<Encoding>, Const> begin,
		string_iterator_impl<boundary_unit<Encoding>, Const> end,
		typename boundary_unit<Encoding>::const_pointer expr)
	{
		return ext::strrfind(begin, end, expr, ext::strend(expr));
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	rfind(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		const_string_iterator<Boundary> exprbegin,
		const_string_iterator<Boundary> exprend)
	{
		return ext::strrfind(begin, end, exprbegin.get(), exprend.get());
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		typename Boundary::value_type c)
	{
		return ext::strbegins(begin.get(), end.get(), c);
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		typename Boundary::const_pointer str)
	{
		return ext::strbegins(begin.get(), end.get(), str);
	}

	template <typename Boundary>
	constexpr const_string_iterator<Boundary>
	starts_with(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		const_string_iterator<Boundary> exprbegin,
		const_string_iterator<Boundary> exprend)
	{
		return ext::strbegins(begin.get(), end.get(), exprbegin.get(), exprend.get());
	}
}
