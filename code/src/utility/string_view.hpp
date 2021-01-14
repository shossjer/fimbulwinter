#pragma once

#include "utility/ext/string.hpp"
#include "utility/string_iterator.hpp"

#include <iostream>

namespace utility
{
	template <typename Boundary>
	class basic_string_view
	{
		using this_type = basic_string_view<Boundary>;

	public:

		using encoding_type = typename Boundary::encoding_type;

		using value_type = typename Boundary::value_type;
		using const_pointer = typename Boundary::const_pointer;
		using const_reference = typename Boundary::const_reference;
		using size_type = typename Boundary::size_type;
		using difference_type = typename Boundary::difference_type;

		using const_iterator = const_string_iterator<Boundary>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:

		const_pointer ptr_;
		size_type size_;

		struct other_offset {};
		struct other_substr {};

	public:

		basic_string_view() = default;

		constexpr basic_string_view(const_iterator begin, const_iterator end)
			: ptr_(begin.get())
			, size_(end - begin)
		{}

		constexpr basic_string_view(const_pointer s)
			: ptr_(s)
			, size_(ext::strlen(s))
		{}

		constexpr basic_string_view(const_pointer begin, const_pointer end)
			: ptr_(begin)
			, size_(end - begin)
		{}

		explicit constexpr basic_string_view(const_pointer s, size_type count)
			: ptr_(s)
			, size_(Boundary::next(s, count))
		{}

		explicit constexpr basic_string_view(const this_type & other, size_type position)
			: basic_string_view(other_offset{}, other, Boundary::next(other.data(), position))
		{}

		explicit constexpr basic_string_view(const this_type & other, size_type position, size_type count)
			: basic_string_view(other_substr{}, other, Boundary::next(other.data(), position), count)
		{}

	private:

		explicit constexpr basic_string_view(other_offset, const this_type & other, size_type offset)
			: ptr_(other.data() + offset)
			, size_(other.size() - offset)
		{}

		explicit constexpr basic_string_view(other_substr, const this_type & other, size_type offset, size_type count)
			: ptr_(other.data() + offset)
			, size_(Boundary::next(other.data() + offset, count))
		{}

	public:

		constexpr const_iterator begin() const { return const_iterator(this->data()); }
		constexpr const_iterator cbegin() const { return const_iterator(this->data()); }
		constexpr const_iterator end() const { return const_iterator(this->data() + this->size()); }
		constexpr const_iterator cend() const { return const_iterator(this->data() + this->size()); }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		constexpr size_type size() const { return size_; }
		constexpr size_type length() const { return Boundary::length(this->data(), this->data() + this->size()); }

		constexpr const_pointer data() const { return ptr_; }

		constexpr const_reference operator [] (size_type position) const { return begin()[position]; }

	public:

		friend constexpr bool operator == (this_type x, this_type y) { return compare(x.begin(), x.end(), y.begin(), y.end()) == 0; }
		friend constexpr bool operator == (this_type x, const value_type * s) { return compare(x.begin(), x.end(), s) == 0; }
		friend constexpr bool operator == (const value_type * s, this_type y) { return compare(y.begin(), y.end(), s) == 0; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator != (this_type x, const value_type * s) { return !(x == s); }
		friend constexpr bool operator != (const value_type * s, this_type y) { return !(s == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return compare(x.begin(), x.end(), y.begin(), y.end()) < 0; }
		friend constexpr bool operator < (this_type x, const value_type * s) { return compare(x.begin(), x.end(), s) < 0; }
		friend constexpr bool operator < (const value_type * s, this_type y) { return compare(y.begin(), y.end(), s) > 0; }
		friend constexpr bool operator <= (this_type x, this_type y) { return !(y < x); }
		friend constexpr bool operator <= (this_type x, const value_type * s) { return !(s < x); }
		friend constexpr bool operator <= (const value_type * s, this_type y) { return !(y < s); }
		friend constexpr bool operator > (this_type x, this_type y) { return y < x; }
		friend constexpr bool operator > (this_type x, const value_type * s) { return s < x; }
		friend constexpr bool operator > (const value_type * s, this_type y) { return y < s; }
		friend constexpr bool operator >= (this_type x, this_type y) { return !(x < y); }
		friend constexpr bool operator >= (this_type x, const value_type * s) { return !(x < s); }
		friend constexpr bool operator >= (const value_type * s, this_type y) { return !(s < y); }

		template <typename Traits>
		friend std::basic_ostream<value_type, Traits> & operator << (std::basic_ostream<value_type, Traits> & os, this_type x)
		{
			return os.write(x.ptr_, x.size_);
		}
	};

	template <typename Boundary>
	constexpr bool empty(basic_string_view<Boundary> view) { return view.begin() == view.end(); }

	template <typename Boundary>
	constexpr int
	compare(
		basic_string_view<Boundary> view,
		typename Boundary::value_type c)
	{
		return utility::compare(view.begin(), view.end(), c);
	}

	template <typename Boundary>
	constexpr int
	compare(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		basic_string_view<Boundary> str)
	{
		return utility::compare(begin, end, str.begin(), str.end());
	}

	template <typename Boundary>
	constexpr int
	compare(
		basic_string_view<Boundary> view,
		typename Boundary::const_pointer str)
	{
		return utility::compare(view.begin(), view.end(), str);
	}

	template <typename Boundary>
	constexpr int
	compare(
		basic_string_view<Boundary> view,
		basic_string_view<Boundary> str)
	{
		return utility::compare(view.begin(), view.end(), str.begin(), str.end());
	}

	template <typename Boundary>
	constexpr const_string_iterator<Boundary>
	find(
		basic_string_view<Boundary> view,
		typename Boundary::const_reference c)
	{
		return utility::find(view.begin(), view.end(), c);
	}

	template <typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	find(
		basic_string_view<boundary_unit<Encoding>> view,
		typename boundary_unit<Encoding>::const_pointer expr)
	{
		return utility::find(view.begin(), view.end(), expr);
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	find(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		basic_string_view<Boundary> expr)
	{
		return utility::find(begin, end, expr.begin(), expr.end());
	}

	template <typename Boundary>
	constexpr const_string_iterator<Boundary>
	find(
		basic_string_view<Boundary> view,
		basic_string_view<Boundary> expr)
	{
		return utility::find(view.begin(), view.end(), expr.begin(), expr.end());
	}

	template <typename Boundary>
	constexpr const_string_iterator<Boundary>
	rfind(
		basic_string_view<Boundary> view,
		typename Boundary::const_reference c)
	{
		return utility::rfind(view.begin(), view.end(), c);
	}

	template <typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	rfind(
		basic_string_view<boundary_unit<Encoding>> view,
		typename boundary_unit<Encoding>::const_pointer expr)
	{
		return utility::rfind(view.begin(), view.end(), expr);
	}

	template <typename Boundary, bool Const>
	constexpr string_iterator_impl<Boundary, Const>
	rfind(
		string_iterator_impl<Boundary, Const> begin,
		string_iterator_impl<Boundary, Const> end,
		basic_string_view<Boundary> expr)
	{
		return utility::rfind(begin, end, expr.begin(), expr.end());
	}

	template <typename Boundary>
	constexpr const_string_iterator<Boundary>
	rfind(
		basic_string_view<Boundary> view,
		basic_string_view<Boundary> expr)
	{
		return utility::rfind(view.begin(), view.end(), expr.begin(), expr.end());
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		basic_string_view<Boundary> view,
		typename Boundary::value_type c)
	{
		return utility::starts_with(view.begin(), view.end(), c);
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		basic_string_view<Boundary> view,
		typename Boundary::const_pointer str)
	{
		return utility::starts_with(view.begin(), view.end(), str);
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		const_string_iterator<Boundary> begin,
		const_string_iterator<Boundary> end,
		basic_string_view<Boundary> expr)
	{
		return utility::starts_with(begin, end, expr.begin(), expr.end());
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		basic_string_view<Boundary> view,
		const_string_iterator<Boundary> exprbegin,
		const_string_iterator<Boundary> exprend)
	{
		return utility::starts_with(view.begin(), view.end(), exprbegin, exprend);
	}

	template <typename Boundary>
	constexpr bool
	starts_with(
		basic_string_view<Boundary> view,
		basic_string_view<Boundary> expr)
	{
		return utility::starts_with(view.begin(), view.end(), expr.begin(), expr.end());
	}
}
