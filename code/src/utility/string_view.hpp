
#ifndef UTILITY_STRING_VIEW_HPP
#define UTILITY_STRING_VIEW_HPP

#include <algorithm>
#include <cstddef>
#include <ostream>

namespace utility
{
	constexpr std::size_t strlen_impl(const char * str, std::size_t len)
	{
		return *str ? strlen_impl(str + 1, len + 1) : len;
	}

	constexpr std::size_t strlen(const char * str)
	{
		return str ? strlen_impl(str, 0) : throw "strlen expects non-null string";
	}

	template <typename T>
	constexpr const T & min(const T & a, const T & b)
	{
		return b < a ? b : a;
	}


	class string_view
	{
	private:
		const char * str;
		std::size_t len;

	public:
		constexpr string_view() noexcept
			: str(nullptr)
			, len(0)
		{}
		constexpr string_view(const string_view &) noexcept = default;
		constexpr string_view(const char * str, std::size_t len)
			: str(str)
			, len(len)
		{}
		constexpr string_view(const char * str)
			: str(str)
			, len(strlen(str))
		{}
		constexpr string_view & operator = (const string_view &) noexcept = default;

	public:
		constexpr const char * begin() const noexcept { return str; }
		constexpr const char * cbegin() const noexcept { return str; }

		constexpr const char * end() const noexcept { return str + len; }
		constexpr const char * cend() const noexcept { return str + len; }

	public:
		constexpr const char & operator [] (int index) const { return str[index]; }

		constexpr const char * data() const { return str; }

	public:
		constexpr std::size_t size() const { return len; }
		constexpr std::size_t length() const { return len; }

		constexpr bool empty() const { return len == 0; }

	public:
		constexpr int compare(string_view that) const noexcept
		{
			return compare_impl(that, compare_data(str, that.str, min(len, that.len)));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, string_view that) const
		{
			return string_view(str + pos1, count1).compare(that);
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, string_view that, std::size_t pos2, std::size_t count2) const
		{
			return string_view(str + pos1, count1).compare(string_view(that.str + pos2, count2));
		}
		constexpr int compare(const char * cstr) const
		{
			return compare(string_view(cstr));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, const char * cstr) const
		{
			return string_view(str + pos1, count1).compare(string_view(cstr));
		}
		constexpr int compare(std::size_t pos1, std::size_t count1, const char * cstr, std::size_t count2) const
		{
			return string_view(str + pos1, count1).compare(string_view(cstr, count2));
		}

		constexpr std::size_t find(string_view that, std::size_t pos = 0) const
		{
			return find_impl(that, len < that.len ? std::size_t(-1) : pos);
		}
		constexpr std::size_t find(const char * str, std::size_t pos = 0) const
		{
			return find(string_view(str), pos);
		}
		constexpr std::size_t rfind(string_view that, std::size_t pos = std::size_t(-1)) const
		{
			return rfind_impl(that, len < that.len ? std::size_t(-1) : std::min(pos, len - that.len));
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
				len < that.len ? -1 :
				that.len < len ? 1 :
				0;
		}

		constexpr std::size_t find_impl(string_view that, std::size_t pos) const
		{
			return pos > len - that.len ? std::size_t(-1) : compare_data(str + pos, that.str, that.len) == 0 ? pos : find_impl(that, pos + 1);
		}
		constexpr std::size_t rfind_impl(string_view that, std::size_t pos) const
		{
			return pos > len ? std::size_t(-1) : compare_data(str + pos, that.str, that.len) == 0 ? pos : rfind_impl(that, pos - 1);
		}
	};

	inline constexpr bool operator == (string_view a, string_view b) noexcept { return a.compare(b) == 0; }
	inline constexpr bool operator != (string_view a, string_view b) noexcept { return a.compare(b) != 0; }
	inline constexpr bool operator < (string_view a, string_view b) noexcept { return a.compare(b) < 0; }
	inline constexpr bool operator <= (string_view a, string_view b) noexcept { return a.compare(b) <= 0; }
	inline constexpr bool operator > (string_view a, string_view b) noexcept { return a.compare(b) > 0; }
	inline constexpr bool operator >= (string_view a, string_view b) noexcept { return a.compare(b) >= 0; }

	inline std::ostream & operator << (std::ostream & s, string_view v)
	{
		for (char c : v)
			s.put(c);

		return s;
	}
}

#endif /* UTILITY_STRING_VIEW_HPP */
