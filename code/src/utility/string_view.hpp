
#ifndef UTILITY_STRING_VIEW_HPP
#define UTILITY_STRING_VIEW_HPP

#include "utility/ext/string.hpp"

#include <algorithm>
#include <ostream>

namespace utility
{
	template <typename T>
	constexpr const T & min(const T & a, const T & b)
	{
		return b < a ? b : a;
	}

	class string_view
	{
	private:
		const char * str_;
		std::size_t len_;

	public:
		constexpr string_view() noexcept
			: str_(nullptr)
			, len_(0)
		{}
		constexpr string_view(const string_view &) noexcept = default;
		constexpr string_view(const char * str, std::size_t len)
			: str_(str)
			, len_(len)
		{}
		constexpr string_view(const char * str)
			: str_(str)
			, len_(ext::strlen(str))
		{}
		constexpr string_view & operator = (const string_view &) noexcept = default;

	public:
		constexpr const char * begin() const noexcept { return str_; }
		constexpr const char * cbegin() const noexcept { return str_; }

		constexpr const char * end() const noexcept { return str_ + len_; }
		constexpr const char * cend() const noexcept { return str_ + len_; }

	public:
		constexpr const char & operator [] (int index) const { return str_[index]; }

		constexpr const char * data() const { return str_; }

	public:
		constexpr std::size_t size() const { return len_; }
		constexpr std::size_t length() const { return len_; }

		constexpr bool empty() const { return len_ == 0; }

	public:
		constexpr int compare(string_view that) const noexcept
		{
			return compare_impl(that, compare_data(str_, that.str_, min(len_, that.len_)));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, string_view that) const
		{
			return string_view(str_ + pos1, count1).compare(that);
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, string_view that, std::size_t pos2, std::size_t count2) const
		{
			return string_view(str_ + pos1, count1).compare(string_view(that.str_ + pos2, count2));
		}
		constexpr int compare(const char * cstr) const
		{
			return compare(string_view(cstr));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, const char * cstr) const
		{
			return string_view(str_ + pos1, count1).compare(string_view(cstr));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, const char * cstr, std::size_t count2) const
		{
			return string_view(str_ + pos1, count1).compare(string_view(cstr, count2));
		}

		constexpr std::size_t find(string_view that, std::size_t pos = 0) const
		{
			return find_impl(that, len_ < that.len_ ? std::size_t(-1) : pos);
		}
		constexpr std::size_t find(const char * str, std::size_t pos = 0) const
		{
			return find(string_view(str), pos);
		}
		constexpr std::size_t rfind(string_view that, std::size_t pos = std::size_t(-1)) const
		{
			return rfind_impl(that, len_ < that.len_ ? std::size_t(-1) : std::min(pos, len_ - that.len_));
		}
		constexpr std::size_t rfind(const char * str, std::size_t pos = std::size_t(-1)) const
		{
			return rfind(string_view(str), pos);
		}
	private:
		constexpr static int compare_data(const char * a, const char * b, std::size_t count)
		{
			return count == 0 ? 0 :
				*a < *b ? -1 :
				*b < *a ? 1 :
				compare_data(a + 1, b + 1, count - 1);
		}
		constexpr int compare_impl(string_view that, int value) const
		{
			return value != 0 ? value :
				len_ < that.len_ ? -1 :
				that.len_ < len_ ? 1 :
				0;
		}

		constexpr std::size_t find_impl(string_view that, std::size_t pos) const
		{
			return pos > len_ - that.len_ ? std::size_t(-1) : compare_data(str_ + pos, that.str_, that.len_) == 0 ? pos : find_impl(that, pos + 1);
		}
		constexpr std::size_t rfind_impl(string_view that, std::size_t pos) const
		{
			return pos > len_ ? std::size_t(-1) : compare_data(str_ + pos, that.str_, that.len_) == 0 ? pos : rfind_impl(that, pos - 1);
		}

		friend constexpr bool operator == (string_view a, string_view b) noexcept { return a.compare(b) == 0; }
		friend constexpr bool operator != (string_view a, string_view b) noexcept { return a.compare(b) != 0; }
		friend constexpr bool operator < (string_view a, string_view b) noexcept { return a.compare(b) < 0; }
		friend constexpr bool operator <= (string_view a, string_view b) noexcept { return a.compare(b) <= 0; }
		friend constexpr bool operator > (string_view a, string_view b) noexcept { return a.compare(b) > 0; }
		friend constexpr bool operator >= (string_view a, string_view b) noexcept { return a.compare(b) >= 0; }

		friend std::ostream & operator << (std::ostream & s, string_view v)
		{
			for (char c : v)
				s.put(c);

			return s;
		}
	};
}

#endif /* UTILITY_STRING_VIEW_HPP */
