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

		constexpr int compare(this_type other) const
		{
			return compare_impl(compare_data(ptr_,
			                                 other.ptr_,
			                                 std::min(size_, other.size_)),
			                    size_,
			                    other.size_);
		}

		constexpr int compare(size_type pos1, size_type count1, this_type other) const
		{
			return compare_impl(compare_data(ptr_ + pos1,
			                                 other.ptr_,
			                                 std::min(count1, other.size_)),
			                    count1,
			                    other.size_);
		}

		constexpr int compare(size_type pos1, size_type count1, this_type other, size_type pos2, size_type count2) const
		{
			return compare_impl(compare_data(ptr_ + pos1,
			                                 other.ptr_ + pos2,
			                                 std::min(count1, count2)),
			                    count1,
			                    count2);
		}

		constexpr int compare(size_type pos1, size_type count1, const_pointer s, size_type count2) const
		{
			return s ? compare_impl(compare_data(ptr_ + pos1, s, std::min(count1, count2)),
			                        count1,
			                        count2) : count1 != 0;
		}

		constexpr ext::index find(value_type c) const
		{
			return ext::strfind(ptr_, ptr_ + size_, c) - ptr_;
		}

		constexpr ext::index find(value_type c, size_type from) const
		{
			return ext::strfind(ptr_ + from, ptr_ + size_, c) - ptr_;
		}

		constexpr ext::index find(this_type that, size_type from = 0) const
		{
			return find_impl(that, size_ < that.size_ ? ext::usize(-1) : from);
		}

		constexpr ext::index rfind(value_type c) const
		{
			return ext::strrfind(ptr_, ptr_ + size_, c) - ptr_;
		}

		constexpr ext::index rfind(this_type that, size_type from = size_type(-1)) const
		{
			return rfind_impl(that, size_ < that.size_ ? size_type(-1) : std::min(from, size_ - that.size_));
		}

	private:

		static constexpr int compare_data(const_pointer a, const_pointer b, size_type count)
		{
			return
				count <= 0 ? 0 :
				*a < *b ? -1 :
				*b < *a ? 1 :
				compare_data(a + 1, b + 1, count - 1);
		}

		static constexpr int compare_data_null_terminated(const_pointer a_from, const_pointer a_to, const_pointer b)
		{
			return
				a_from == a_to ? (*b == 0 ? 0 : -1) :
				*a_from < *b ? -1 :
				*b < *a_from ? 1 :
				compare_data_null_terminated(a_from + 1, a_to, b + 1);
		}

		static constexpr int compare_impl(int res, size_type counta, size_type countb)
		{
			return
				res != 0 ? res :
				counta < countb ? -1 :
				countb < counta ? 1 :
				0;
		}

		constexpr ext::index find_impl(this_type that, size_type from) const
		{
			return from > size_ - that.size_ ? ext::index_invalid : compare_data(ptr_ + from, that.ptr_, that.size_) == 0 ? from : find_impl(that, from + 1);
		}

		constexpr ext::index rfind_impl(this_type that, size_type from) const
		{
			return from > size_ ? ext::index_invalid : compare_data(ptr_ + from, that.ptr_, that.size_) == 0 ? from : rfind_impl(that, from - 1);
		}

	public:

		friend constexpr bool operator == (this_type x, this_type y) { return x.compare(y) == 0; }
		friend constexpr bool operator == (this_type x, const value_type * s) { return x.compare(s) == 0; }
		friend constexpr bool operator == (const value_type * s, this_type y) { return y.compare(s) == 0; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator != (this_type x, const value_type * s) { return !(x == s); }
		friend constexpr bool operator != (const value_type * s, this_type y) { return !(s == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return x.compare(y) < 0; }
		friend constexpr bool operator < (this_type x, const value_type * s) { return x.compare(s) < 0; }
		friend constexpr bool operator < (const value_type * s, this_type y) { return y.compare(s) > 0; }
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
}
