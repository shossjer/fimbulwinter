#pragma once

#include "array_alloc.hpp"
#include "intrinsics.hpp"
#include "arithmetics.hpp"

#include <cstring>
#include <iostream>

namespace utility
{
	using code_unit_utf8 = char;
	using code_unit_utf16 = char16_t;

	class code_point
	{
	private:
		uint32_t value_ : 21;
		uint32_t size_ : 3;

	public:
		code_point() = default;
		explicit constexpr code_point(std::nullptr_t)
			: value_(0)
			, size_(1)
		{}
		explicit constexpr code_point(int32_t value)
			: value_(value)
			, size_(value < 0x80 ? 1 : value < 0x800 ? 2 : value < 0x10000 ? 3 : 4)
		{}
		explicit constexpr code_point(uint32_t value)
			: value_(value)
			, size_(value < 0x80 ? 1 : value < 0x800 ? 2 : value < 0x10000 ? 3 : 4)
		{}
		explicit constexpr code_point(const code_unit_utf8 * s)
			: code_point(s, s ? extract_size(s) : 1)
		{}
		explicit constexpr code_point(const code_unit_utf16 * s)
			: code_point(s ? extract_value(s, (s[0] & 0xfc00) == 0xd800 ? 2 : 1) : 0)
		{}
	private:
		constexpr code_point(const code_unit_utf8 * s, uint32_t size)
			: value_(s ? extract_value(s, size) : 0)
			, size_(size)
		{}

	public:
		constexpr int get() const { return value_; }
		constexpr int size() const { return size_; }

		void get(code_unit_utf8 * s) const
		{
			switch (size_)
			{
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xf0 = 1111 0000
			case 4: s[3] = (value_ & 0x3f) | 0x80; s[2] = ((value_ >> 6) & 0x3f) | 0x80; s[1] = ((value_ >> 12) & 0x3f) | 0x80; s[0] = (value_ >> 18) | 0xf0; break;
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xe0 = 1110 0000
			case 3: s[2] = (value_ & 0x3f) | 0x80; s[1] = ((value_ >> 6) & 0x3f) | 0x80; s[0] = (value_ >> 12) | 0xe0; break;
				// 0x3f = 0011 1111
				// 0x80 = 1000 0000
				// 0xc0 = 1100 0000
			case 2: s[1] = (value_ & 0x3f) | 0x80; s[0] = (value_ >> 6) | 0xc0; break;
			case 1: s[0] = value_; break;
			default: intrinsic_unreachable();
			}
		}

	public:
		friend constexpr bool operator == (code_point a, code_point b) { return a.value_ == b.value_; }
		friend constexpr bool operator != (code_point a, code_point b) { return !(a == b); }
		friend constexpr bool operator < (code_point a, code_point b) { return a.value_ < b.value_; }
		friend constexpr bool operator <= (code_point a, code_point b) { return !(b < a); }
		friend constexpr bool operator > (code_point a, code_point b) { return b < a; }
		friend constexpr bool operator >= (code_point a, code_point b) { return !(a < b); }

		template <typename Traits>
		friend std::basic_ostream<code_unit_utf8, Traits> & operator << (std::basic_ostream<code_unit_utf8, Traits> & os, code_point x)
		{
			code_unit_utf8 chars[4];
			x.get(chars);

			return os.write(chars, x.size_);
		}

	public:
		static constexpr int count(const code_unit_utf8 * from, const code_unit_utf8 * to)
		{
			return count_impl(from, to, 0);
		}
		static constexpr int count_impl(const code_unit_utf8 * from, const code_unit_utf8 * to, int length)
		{
			return from < to ? count_impl(from + next(from), to, length + 1) : length;
		}

		static constexpr int next(const code_unit_utf8 * s)
		{
			return extract_size(s);
		}

		static constexpr int previous(const code_unit_utf8 * s)
		{
			return previous_impl(s - 1, s);
		}
		static constexpr int previous_impl(const code_unit_utf8 * s, const code_unit_utf8 * from)
		{
			// 0x80 = 1000 0000
			// 0xc0 = 1100 0000
			return (*s & 0xc0) == 0x80 ? previous_impl(s - 1, from) : from - s;
		}

		static constexpr int previous(const code_unit_utf8 * s, int length)
		{
			return previous_impl(s, length, s);
		}
		static constexpr int previous_impl(const code_unit_utf8 * s, int length, const code_unit_utf8 * from)
		{
			return length <= 0 ? from - s : previous_impl(s - previous(s), length - 1, from);
		}

		static constexpr int next(const code_unit_utf8 * s, int length)
		{
			return next_impl(s, length, s);
		}
		static constexpr int next_impl(const code_unit_utf8 * s, int length, const code_unit_utf8 * from)
		{
			return length <= 0 ? s - from : next_impl(s + next(s), length - 1, from);
		}

		static constexpr int extract_size(const code_unit_utf8 * s)
		{
			constexpr int size_table[16] = {
				1, 1, 1, 1,
				1, 1, 1, 1,
				-1, -1, -1, -1,
				2, 2, 3, 4,
			};
			return size_table[uint8_t(s[0]) >> 4];
		}

		static constexpr int extract_value(const code_unit_utf8 * s, int size)
		{
			switch (size)
			{
				// 0x07 = 0000 0111
				// 0x3f = 0011 1111
			case 4: return uint32_t(s[0] & 0x07) << 18 | uint32_t(s[1] & 0x3f) << 12 | uint32_t(s[2] & 0x3f) << 6 | uint32_t(s[3] & 0x3f);
				// 0x0f = 0000 1111
				// 0x3f = 0011 1111
			case 3: return uint32_t(s[0] & 0x0f) << 12 | uint32_t(s[1] & 0x3f) << 6 | uint32_t(s[2] & 0x3f);
				// 0x1f = 0001 1111
				// 0x3f = 0011 1111
			case 2: return uint32_t(s[0] & 0x1f) << 6 | uint32_t(s[1] & 0x3f);
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}
		static constexpr int extract_value(const code_unit_utf16 * s, int size)
		{
			switch (size)
			{
				// 0x03ff = 0000 0011 1111 1111
			case 2: return (uint32_t(s[0] & 0x03ff) << 10 | uint32_t(s[1] & 0x03ff)) + 0x10000;
			case 1: return s[0];
			default: intrinsic_unreachable();
			}
		}
	};

	struct PointDifference : Arithmetic<PointDifference, std::ptrdiff_t>
	{
		using Arithmetic<PointDifference, std::ptrdiff_t>::Arithmetic;
	};

	struct UnitDifference : Arithmetic<UnitDifference, std::ptrdiff_t>
	{
		using Arithmetic<UnitDifference, std::ptrdiff_t>::Arithmetic;
	};

	class LazyDifference
	{
	private:
		const code_unit_utf8 * begin_;
		const code_unit_utf8 * end_;

	public:
		constexpr LazyDifference(const code_unit_utf8 * begin, const code_unit_utf8 * end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr operator PointDifference () const
		{
			if (end_ < begin_)
				return PointDifference(-code_point::count(end_, begin_));
			return PointDifference(code_point::count(begin_, end_));
		}
		constexpr operator UnitDifference () const { return UnitDifference(end_ - begin_); }
	};

	class Utf8ConstIterator;
	class string_view_utf8;
	template <typename StorageTraits>
	class Utf8String;

	class Utf8Iterator
	{
		friend class Utf8ConstIterator;
		template <typename StorageTraits>
		friend class Utf8String;

	public:
		using difference_type = LazyDifference;
		using difference_unit_type = UnitDifference;
		using difference_point_type = PointDifference;
		using value_type = code_unit_utf8;
		using pointer = code_unit_utf8 *;
		using reference = code_point;
		using iterator_category = std::bidirectional_iterator_tag;

	private:
		pointer ptr_;

	public:
		Utf8Iterator() = default;
	private:
		constexpr Utf8Iterator(pointer ptr) : ptr_(ptr) {}

	public:
		constexpr reference operator * () const { return code_point(ptr_); }

		constexpr Utf8Iterator & operator ++ () { ptr_ += code_point::next(ptr_); return *this; }
		constexpr Utf8Iterator operator ++ (int) { auto tmp = *this; ptr_ += code_point::next(ptr_); return tmp; }
		constexpr Utf8Iterator & operator -- () { ptr_ -= code_point::previous(ptr_); return *this; }
		constexpr Utf8Iterator operator -- (int) { auto tmp = *this; ptr_ -= code_point::previous(ptr_); return tmp; }

		constexpr Utf8Iterator & operator += (difference_unit_type n) { ptr_ += n.get(); return *this; }
		constexpr Utf8Iterator & operator += (difference_point_type n) { ptr_ += code_point::next(ptr_, n.get()); return *this; }
		constexpr Utf8Iterator & operator -= (difference_unit_type n) { ptr_ -= n.get(); return *this; }
		constexpr Utf8Iterator & operator -= (difference_point_type n) { ptr_ -= code_point::previous(ptr_, n.get()); return *this; }

		constexpr reference operator [] (difference_unit_type n) const { return code_point(ptr_ + n.get()); }
		constexpr reference operator [] (difference_point_type n) const { return code_point(ptr_ + code_point::next(ptr_, n.get())); }

		constexpr code_unit_utf8 * get() { return ptr_; }
		constexpr const code_unit_utf8 * get() const { return ptr_; }

		friend constexpr Utf8Iterator operator + (Utf8Iterator x, difference_unit_type n) { return x += n; }
		friend constexpr Utf8Iterator operator + (difference_unit_type n, Utf8Iterator x) { return x += n; }
		friend constexpr Utf8Iterator operator + (Utf8Iterator x, difference_point_type n) { return x += n; }
		friend constexpr Utf8Iterator operator + (difference_point_type n, Utf8Iterator x) { return x += n; }
		friend constexpr Utf8Iterator operator - (Utf8Iterator x, difference_unit_type n) { return x -= n; }
		friend constexpr Utf8Iterator operator - (Utf8Iterator x, difference_point_type n) { return x -= n; }

		friend constexpr bool operator == (Utf8Iterator x, Utf8Iterator y) { return x.ptr_ == y.ptr_; }
		friend constexpr bool operator != (Utf8Iterator x, Utf8Iterator y) { return !(x == y); }
		friend constexpr bool operator < (Utf8Iterator x, Utf8Iterator y) { return x.ptr_ < y.ptr_; }
		friend constexpr bool operator <= (Utf8Iterator x, Utf8Iterator y) { return !(y < x); }
		friend constexpr bool operator > (Utf8Iterator x, Utf8Iterator y) { return y < x; }
		friend constexpr bool operator >= (Utf8Iterator x, Utf8Iterator y) { return !(x < y); }

		friend constexpr difference_type operator - (Utf8Iterator x, Utf8Iterator y) { return {y.ptr_, x.ptr_}; }
	};

	class Utf8ConstIterator
	{
		friend class string_view_utf8;
		template <typename StorageTraits>
		friend class Utf8String;

	public:
		using difference_type = LazyDifference;
		using difference_unit_type = UnitDifference;
		using difference_point_type = PointDifference;
		using value_type = code_unit_utf8;
		using pointer = const code_unit_utf8 *;
		using reference = code_point;
		using iterator_category = std::bidirectional_iterator_tag;

	private:
		pointer ptr_;

	public:
		Utf8ConstIterator() = default;
		constexpr Utf8ConstIterator(Utf8Iterator it) : ptr_(it.ptr_) {}
	private:
		explicit constexpr Utf8ConstIterator(pointer ptr) : ptr_(ptr) {}

	public:
		constexpr reference operator * () const { return code_point(ptr_); }

		constexpr Utf8ConstIterator & operator ++ () { ptr_ += code_point::next(ptr_); return *this; }
		constexpr Utf8ConstIterator operator ++ (int) { auto tmp = *this; ptr_ += code_point::next(ptr_); return tmp; }
		constexpr Utf8ConstIterator & operator -- () { ptr_ -= code_point::previous(ptr_); return *this; }
		constexpr Utf8ConstIterator operator -- (int) { auto tmp = *this; ptr_ -= code_point::previous(ptr_); return tmp; }

		constexpr Utf8ConstIterator & operator += (difference_unit_type n) { ptr_ += n.get(); return *this; }
		constexpr Utf8ConstIterator & operator += (difference_point_type n) { ptr_ += code_point::next(ptr_, n.get()); return *this; }
		constexpr Utf8ConstIterator & operator -= (difference_unit_type n) { ptr_ -= n.get(); return *this; }
		constexpr Utf8ConstIterator & operator -= (difference_point_type n) { ptr_ -= code_point::previous(ptr_, n.get()); return *this; }

		constexpr reference operator [] (difference_unit_type n) const { return code_point(ptr_ + n.get()); }
		constexpr reference operator [] (difference_point_type n) const { return code_point(ptr_ + code_point::next(ptr_, n.get())); }

		constexpr const code_unit_utf8 * get() { return ptr_; }
		constexpr const code_unit_utf8 * get() const { return ptr_; }

		friend constexpr Utf8ConstIterator operator + (Utf8ConstIterator x, difference_unit_type n) { return x += n; }
		friend constexpr Utf8ConstIterator operator + (difference_unit_type n, Utf8ConstIterator x) { return x += n; }
		friend constexpr Utf8ConstIterator operator + (Utf8ConstIterator x, difference_point_type n) { return x += n; }
		friend constexpr Utf8ConstIterator operator + (difference_point_type n, Utf8ConstIterator x) { return x += n; }
		friend constexpr Utf8ConstIterator operator - (Utf8ConstIterator x, difference_unit_type n) { return x -= n; }
		friend constexpr Utf8ConstIterator operator - (Utf8ConstIterator x, difference_point_type n) { return x -= n; }

		friend constexpr bool operator == (Utf8ConstIterator x, Utf8ConstIterator y) { return x.ptr_ == y.ptr_; }
		friend constexpr bool operator != (Utf8ConstIterator x, Utf8ConstIterator y) { return !(x == y); }
		friend constexpr bool operator < (Utf8ConstIterator x, Utf8ConstIterator y) { return x.ptr_ < y.ptr_; }
		friend constexpr bool operator <= (Utf8ConstIterator x, Utf8ConstIterator y) { return !(y < x); }
		friend constexpr bool operator > (Utf8ConstIterator x, Utf8ConstIterator y) { return y < x; }
		friend constexpr bool operator >= (Utf8ConstIterator x, Utf8ConstIterator y) { return !(x < y); }

		friend constexpr difference_type operator - (Utf8ConstIterator x, Utf8ConstIterator y) { return {y.ptr_, x.ptr_}; }
	};

	class string_view_utf8
	{
	public:
		using difference_type = LazyDifference;
		using difference_unit_type = UnitDifference;
		using difference_point_type = PointDifference;
		using value_type = code_unit_utf8;
		using pointer = code_unit_utf8 *;
		using reference = code_point;
		using size_type = std::size_t;

		using const_iterator = Utf8ConstIterator;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:
		const_iterator ptr_;
		difference_unit_type size_;

	public:
		string_view_utf8() = default;
		constexpr string_view_utf8(const value_type * s) : string_view_utf8(s, std::strlen(s)) {}
		constexpr string_view_utf8(const value_type * s, size_type count) : ptr_(s), size_(count) {}
		constexpr string_view_utf8(const_iterator ptr, difference_unit_type size) : ptr_(ptr), size_(size) {}
		constexpr string_view_utf8(const_iterator ptr, difference_point_type length) : string_view_utf8(ptr, difference_unit_type(code_point::next(ptr.get(), length.get()))) {}

	public:
		constexpr const_iterator begin() const { return ptr_; }
		constexpr const_iterator cbegin() const { return ptr_; }
		constexpr const_iterator end() const { return ptr_ + size_; }
		constexpr const_iterator cend() const { return ptr_ + size_; }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		constexpr code_point operator [] (difference_unit_type pos) const { return ptr_[pos]; }
		constexpr code_point operator [] (difference_point_type pos) const { return ptr_[pos]; }
		constexpr code_point front() const { return ptr_[difference_unit_type(0)]; }
		constexpr code_point back() const { return ptr_[size_ - code_point::previous(ptr_.get() + size_.get())]; }
		constexpr const value_type * data() const { return ptr_.get(); }

		constexpr difference_unit_type size() const { return size_; }
		constexpr difference_point_type length() const { return difference_point_type(code_point::count(ptr_.get(), ptr_.get() + size_.get())); }
		constexpr bool empty() const { return size_ == 0; }

		void remove_prefix(difference_unit_type n) { ptr_ += n; size_ -= n; }
		void remove_prefix(difference_point_type n) { remove_prefix(difference_unit_type(code_point::next(ptr_.get(), n.get()))); }
		void remove_suffix(difference_unit_type n) { size_ -= n; }
		void remove_suffix(difference_point_type n) { remove_suffix(difference_unit_type(code_point::previous(ptr_.get() + size_.get(), n.get()))); }

		constexpr int compare(string_view_utf8 other) const { return compare_impl(compare_data(ptr_.get(), other.ptr_.get(), std::min(size_.get(), other.size_.get())), size_.get(), other.size_.get()); }
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, string_view_utf8 other) const { return compare_impl(compare_data(ptr_.get() + pos1, other.ptr_.get(), std::min(count1, other.size_.get())), count1, other.size_.get()); }
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, string_view_utf8 other, std::ptrdiff_t pos2, std::ptrdiff_t count2) const { return compare_impl(compare_data(ptr_.get() + pos1, other.ptr_.get() + pos2, std::min(count1, count2)), count1, count2); }
		constexpr int compare(const value_type * s) const { return s ? compare_data_null_terminated(ptr_.get(), ptr_.get() + size_.get(), s) : size_ != 0; }
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, const value_type * s) const { return s ? compare_data_null_terminated(ptr_.get() + pos1, ptr_.get() + pos1 + count1, s) : count1 != 0; }
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, const value_type * s, std::ptrdiff_t count2) const { return s ? compare_impl(compare_data(ptr_.get() + pos1, s, std::min(count1, count2)), count1, count2) : count1 != 0; }

		constexpr bool starts_with(code_point cp) const { return front() == cp; }
		constexpr bool ends_with(code_point cp) const { return back() == cp; }
	private:
		static constexpr int compare_data(const value_type * a, const value_type * b, std::ptrdiff_t count)
		{
			return
				count <= 0 ? 0 :
				*a < *b ? -1 :
				*b < *a ? 1 :
				compare_data(a + 1, b + 1, count - 1);
		}
		static constexpr int compare_data_null_terminated(const value_type * a_from, const value_type * a_to, const value_type * b)
		{
			return
				a_from == a_to ? (*b == 0 ? 0 : -1) :
				*a_from < *b ? -1 :
				*b < *a_from ? 1 :
				compare_data_null_terminated(a_from + 1, a_to, b + 1);
		}
		static constexpr int compare_impl(int res, ptrdiff_t counta, std::ptrdiff_t countb)
		{
			return
				res != 0 ? res :
				counta < countb ? -1 :
				countb < counta ? 1 :
				0;
		}

	public:
		friend constexpr bool operator == (string_view_utf8 x, string_view_utf8 y) { return x.compare(y) == 0; }
		friend constexpr bool operator == (string_view_utf8 x, const value_type * s) { return x.compare(s) == 0; }
		friend constexpr bool operator == (const value_type * s, string_view_utf8 y) { return y.compare(s) == 0; }
		friend constexpr bool operator != (string_view_utf8 x, string_view_utf8 y) { return !(x == y); }
		friend constexpr bool operator != (string_view_utf8 x, const value_type * s) { return !(x == s); }
		friend constexpr bool operator != (const value_type * s, string_view_utf8 y) { return !(s == y); }
		friend constexpr bool operator < (string_view_utf8 x, string_view_utf8 y) { return x.compare(y) < 0; }
		friend constexpr bool operator < (string_view_utf8 x, const value_type * s) { return x.compare(s) < 0; }
		friend constexpr bool operator < (const value_type * s, string_view_utf8 y) { return y.compare(s) > 0; }
		friend constexpr bool operator <= (string_view_utf8 x, string_view_utf8 y) { return !(y < x); }
		friend constexpr bool operator <= (string_view_utf8 x, const value_type * s) { return !(s < x); }
		friend constexpr bool operator <= (const value_type * s, string_view_utf8 y) { return !(y < s); }
		friend constexpr bool operator > (string_view_utf8 x, string_view_utf8 y) { return y < x; }
		friend constexpr bool operator > (string_view_utf8 x, const value_type * s) { return s < x; }
		friend constexpr bool operator > (const value_type * s, string_view_utf8 y) { return y < s; }
		friend constexpr bool operator >= (string_view_utf8 x, string_view_utf8 y) { return !(x < y); }
		friend constexpr bool operator >= (string_view_utf8 x, const value_type * s) { return !(x < s); }
		friend constexpr bool operator >= (const value_type * s, string_view_utf8 y) { return !(s < y); }

		template <typename Traits>
		friend std::basic_ostream<value_type, Traits> & operator << (std::basic_ostream<value_type, Traits> & os, string_view_utf8 x)
		{
			return os.write(x.ptr_.get(), x.size_.get());
		}
	};

	template <std::size_t N>
	using size_type_for =
		mpl::conditional_t<(N < 0x100), std::uint8_t,
		mpl::conditional_t<(N < 0x10000), std::uint16_t,
		mpl::conditional_t<(N < 0x100000000), std::uint32_t, std::uint64_t>>>;

	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		struct Utf8StringStorageDataImpl
		{
			using storage_traits = utility::storage_traits<Storage>;

			using size_type = size_type_for<storage_traits::capacity_value>;

			size_type size_ = 0;
			Storage chars_;

			void initialize()
			{
				set_capacity(storage_traits::capacity_for(1));
				chars_.allocate(capacity());

				set_size(1);
				chars_.construct_at(0, '\0');
			}

			void set_capacity(size_type capacity)
			{
				assert(capacity == storage_traits::capacity_value);
			}
			void set_size(size_type size)
			{
				size_ = size;
			}

			constexpr size_type capacity() const { return storage_traits::capacity_value; }
			size_type size() const { return size_; }
			size_type size_without_null() const { return size_ - 1; }
		};
		template <typename Storage>
		struct Utf8StringStorageDataImpl<Storage, false /*static capacity*/>
		{
			using size_type = std::size_t;
			using storage_traits = utility::storage_traits<Storage>;

			// private:
			// struct Big
			// {
			// 	size_type capacity_1 : CHAR_BITS;
			// 	size_type capacity_2 : (sizeof(size_type) - 1) * CHAR_BITS;
			// 	array_alloc<code_unit, dynamic_alloc> alloc_;
			// 	size_type size_;
			// };
			// struct Small
			// {
			// 	size_type size_ : CHAR_BITS;
			// 	code_unit buffer[sizeof(Big) - 1];
			// };

			// union
			// {
			// 	Small small;
			// 	Big big;
			// };

			size_type capacity_ = 0;
			size_type size_ = 0;
			Storage chars_;

			void initialize()
			{
				set_capacity(storage_traits::capacity_for(1));
				chars_.allocate(capacity());

				set_size(1);
				chars_.construct_at(0, '\0');
			}

			void set_capacity(size_type capacity)
			{
				capacity_ = capacity;
			}
			void set_size(size_type size)
			{
				size_ = size;
			}

			size_type capacity() const { return capacity_; }
			size_type size() const { return size_; }
			size_type size_without_null() const { return size_ - 1; }
		};
	}

	template <typename Storage>
	struct Utf8StringStorageData
		: detail::Utf8StringStorageDataImpl<Storage>
	{
		using is_trivially_destructible = storage_is_trivially_destructible<Storage>;
		using is_trivially_copy_constructible = storage_is_copy_constructible<Storage>;
		using is_trivially_copy_assignable = storage_is_copy_assignable<Storage>;
		using is_trivially_move_constructible = storage_is_trivially_move_constructible<Storage>;
		using is_trivially_move_assignable = storage_is_trivially_move_assignable<Storage>;

		using this_type = Utf8StringStorageData<Storage>;

		bool allocate_storage(std::size_t capacity)
		{
			return this->chars_.allocate(capacity);
		}

		void deallocate_storage(std::size_t capacity)
		{
			this->chars_.deallocate(capacity);
		}

		void copy_construct_range(std::ptrdiff_t index, const this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->chars_.construct_range(index, other.chars_.data() + from, other.chars_.data() + to);
		}

		void move_construct_range(std::ptrdiff_t index, this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->chars_.construct_range(index, std::make_move_iterator(other.chars_.data() + from), std::make_move_iterator(other.chars_.data() + to));
		}

		void destruct_range(std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->chars_.destruct_range(from, to);
		}
	};

	template <typename StorageTraits>
	class Utf8String
	{
	public:
		using value_type = code_unit_utf8;
		using length_type = std::size_t;
		using size_type = std::size_t;
		using difference_type = LazyDifference;
		using difference_unit_type = UnitDifference;
		using difference_point_type = PointDifference;
		using iterator = Utf8Iterator;
		using const_iterator = Utf8ConstIterator;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	private:
		using StorageData = Utf8StringStorageData<typename StorageTraits::template storage_type<value_type>>;
		using Utf8StringStorage = array_wrapper<StorageData>;

	private:
		Utf8StringStorage storage_;

	public:
		Utf8String()
			: storage_(1)
		{
			storage_.set_size(1);
			storage_.chars_.construct_at(0, '\0');
		}
		Utf8String(length_type count, code_point cp)
			: storage_(count * cp.size() + 1)
		{
			code_unit_utf8 chars[4];
			cp.get(chars);

			const std::size_t len = count * cp.size();
			storage_.set_size(len + 1);
			for (int i = 0; i < len; i++)
			{
				storage_.chars_.construct_at(i, chars[i % cp.size()]);
			}
			storage_.chars_.construct_at(len, '\0');
		}
		Utf8String(const value_type * s)
			: Utf8String(s, std::strlen(s))
		{}
		Utf8String(const value_type * s, size_type count)
			: storage_(count + 1)
		{
			storage_.set_size(count + 1);
			storage_.chars_.construct_range(0, s, s + count);
			storage_.chars_.construct_at(count, '\0');
		}
		Utf8String(const Utf8String & other, size_type pos)
			: Utf8String(other.data() + pos, other.storage_.size_without_null() - pos)
		{}
		Utf8String(const Utf8String & other, size_type pos, difference_unit_type count)
			: Utf8String(other.data() + pos, count.get())
		{}
		Utf8String(const Utf8String & other, size_type pos, difference_point_type count)
			: Utf8String(other.data() + pos, code_point::next(other.data() + pos, count.get()))
		{}
		Utf8String & operator = (const value_type * s)
		{
			const auto ret = storage_.try_replace_with(
				std::strlen(s) + 1,
				[&](StorageData & new_data)
				{
					new_data.chars_.construct_range(0, s, s + storage_.size());
				});
			assert(ret);
			return *this;
		}
	private:
		Utf8String(std::size_t size)
			: storage_(size)
		{}

	public:
		const_iterator begin() const { return const_iterator(data()); }
		const_iterator cbegin() const { return const_iterator(data()); }
		const_iterator end() const { return const_iterator(data() + storage_.size_without_null()); }
		const_iterator cend() const { return const_iterator(data() + storage_.size_without_null()); }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		code_point operator [] (difference_unit_type pos) const { return begin()[pos]; }
		code_point operator [] (difference_point_type pos) const { return begin()[pos]; }
		code_point front() const { return begin()[difference_unit_type(0)]; }
		code_point back() const { return begin()[difference_unit_type(storage_.size_without_null() - code_point::previous(data() + storage_.size_without_null()))]; }
		value_type * data() { return storage_.chars_.data(); }
		const value_type * data() const { return storage_.chars_.data(); }
		const value_type * c_str() const { return storage_.chars_.data(); }
		operator string_view_utf8 () const { return {begin(), difference_unit_type(storage_.size_without_null())}; }

		size_type capacity() const { return storage_.capacity(); }
		difference_unit_type size() const { return difference_unit_type(storage_.size_without_null()); }
		difference_point_type length() const { return end() - begin(); }
		bool empty() const { return storage_.size() <= 1; }

		void clear()
		{
			storage_.chars_.destruct_range(0, storage_.size());
			storage_.set_size(1);
			storage_.chars_.construct_at(0, '\0');
		}

		void push_back(code_point cp)
		{
			code_unit_utf8 chars[4];
			cp.get(chars);

			append(chars, cp.size());
		}
		void pop_back()
		{
			storage_.set_size(storage_.size() - code_point::previous(data() + storage_.size_without_null()));
			data()[storage_.size() - 1] = '\0';
		}

		void append(code_point cp) { push_back(cp); }
		void append(const value_type * s) { append(s, std::strlen(s)); }
		void append(const value_type * s, size_type count)
		{
			storage_.grow(count);
			storage_.chars_.destruct_at(storage_.size() - 1);
			storage_.set_size(storage_.size() + count);
			storage_.chars_.construct_range(storage_.size() - 1 - count, s, s + count);
			storage_.chars_.construct_at(storage_.size() - 1, '\0');
		}
		void append(const Utf8String & other) { append(other.data(), other.storage_.size_without_null()); }
		void append(const Utf8String & other, size_type pos) { append(other.data() + pos, other.storage_.size_without_null() - pos); }
		void append(const Utf8String & other, size_type pos, size_type count) { append(other.data() + pos, count); }
		Utf8String & operator += (const value_type * s) { append(s); return *this; }
		Utf8String & operator += (const Utf8String & other) { append(other); return *this; }

		int compare(const value_type * s) const { return compare_data(data(), s); }
	private:
		static int compare_data(const value_type * a, const value_type * b)
		{
			return
				*a < *b ? -1 :
				*b < *a ? 1 :
				*a == '\0' ? 0 :
				compare_data(a + 1, b + 1);
		}

	public:
		friend Utf8String operator + (const Utf8String & x, const value_type * s) { return concat(x.data(), x.storage_.size_without_null(), s, std::strlen(s)); }
		friend Utf8String operator + (const Utf8String & x, const Utf8String & other) { return concat(x.data(), x.storage_.size_without_null(), other.data(), other.storage_.size_without_null()); }
		friend Utf8String operator + (const value_type * s, const Utf8String & x) { return concat(s, std::strlen(s), x.data(), x.storage_.size_without_null()); }
		friend Utf8String operator + (Utf8String && x, const Utf8String & other)
		{
			Utf8String ret = std::move(x);
			ret.append(other);
			return ret;
		}
		friend Utf8String operator + (Utf8String && x, const value_type * s)
		{
			Utf8String ret = std::move(x);
			ret.append(s);
			return ret;
		}

		friend bool operator == (const Utf8String & x, const value_type * s) { return compare_data(x.data(), s) == 0; }
		friend bool operator == (const Utf8String & x, const Utf8String & other) { return compare_data(x.data(), other.data()) == 0; }
		friend bool operator == (const value_type * s, const Utf8String & x) { return compare_data(s, x.data()) == 0; }
		friend bool operator != (const Utf8String & x, const value_type * s) { return !(x == s); }
		friend bool operator != (const Utf8String & x, const Utf8String & other) { return !(x == other); }
		friend bool operator != (const value_type * s, const Utf8String & x) { return !(s == x); }
		friend bool operator < (const Utf8String & x, const value_type * s) { return compare_data(x.data(), s) < 0; }
		friend bool operator < (const Utf8String & x, const Utf8String & other) { return compare_data(x.data(), other.data()) < 0; }
		friend bool operator < (const value_type * s, const Utf8String & x) { return compare_data(s, x.data()) < 0; }
		friend bool operator <= (const Utf8String & x, const value_type * s) { return !(s < x); }
		friend bool operator <= (const Utf8String & x, const Utf8String & other) { return !(other < x); }
		friend bool operator <= (const value_type * s, const Utf8String & x) { return !(x < s); }
		friend bool operator > (const Utf8String & x, const value_type * s) { return s < x; }
		friend bool operator > (const Utf8String & x, const Utf8String & other) { return other < x; }
		friend bool operator > (const value_type * s, const Utf8String & x) { return x < s; }
		friend bool operator >= (const Utf8String & x, const value_type * s) { return !(x < s); }
		friend bool operator >= (const Utf8String & x, const Utf8String & other) { return !(x < other); }
		friend bool operator >= (const value_type * s, const Utf8String & x) { return !(s < x); }

		template <typename Traits>
		friend std::basic_ostream<value_type, Traits> & operator << (std::basic_ostream<value_type, Traits> & os, const Utf8String & x)
		{
			return os.write(x.data(), x.storage_.size_without_null());
		}
	private:
		static Utf8String concat(const value_type * s, std::size_t ls, const value_type * t, std::size_t lt)
		{
			Utf8String ret(ls + lt + 1);
			ret.storage_.set_size(ls + lt + 1);
			ret.storage_.chars_.construct_range(0, s, s + ls);
			ret.storage_.chars_.construct_range(ls, t, t + lt);
			ret.storage_.chars_.construct_at(ls + lt, '\0');
			return ret;
		}
	};

	using heap_string_utf8 = Utf8String<utility::heap_storage_traits>;
	template <std::size_t Capacity>
	using static_string_utf8 = Utf8String<utility::static_storage_traits<Capacity>>;
}
