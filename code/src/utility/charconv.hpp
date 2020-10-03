#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <system_error>

namespace ext
{
	struct from_chars_result
	{
		const char * ptr;
		std::errc ec;
	};

	namespace detail
	{
		struct integer_pattern_base10
		{
			int base;

			const char * match(const char * begin, const char * end) const
			{
				for (; begin != end && (*begin >= '0' && *begin <= '0' + (base - 1)); begin++);
				return begin;
			}

			int value(char c) const
			{
				return c - '0';
			}
		};

		struct integer_pattern_base36
		{
			int base;

			const char * match(const char * begin, const char * end) const
			{
				for (; begin != end && ((*begin >= '0' && *begin <= '9') ||
				                        (*begin >= 'A' && *begin <= 'A' + (base - 11)) ||
				                        (*begin >= 'a' && *begin <= 'a' + (base - 11))); begin++);
				return begin;
			}

			int value(char c) const
			{
				return
					c <= '9' ? c - '0' :
					c <= 'A' ? c - 'A' :
					c - 'a';
			}
		};

		template <typename T>
		struct digits
		{
			static constexpr std::ptrdiff_t max(int base) { return max_impl(base, std::integral_constant<std::size_t, sizeof(T)>{}, std::is_signed<T>{}); }
			static constexpr std::ptrdiff_t min(int base) { return min_impl(base, std::integral_constant<std::size_t, sizeof(T)>{}, std::is_signed<T>{}); }

		private:
			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 1>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 7, 5, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 1>, std::false_type)
			{
				constexpr int counts[] = {-1, -1, 8, 6, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 2>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 15, 10, 8, 7, 6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 2>, std::false_type)
			{
				constexpr int counts[] = {-1, -1, 16, 11, 8, 7, 7, 6, 6, 6, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 4>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 31, 20, 16, 14, 12, 12, 11, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 4>, std::false_type)
			{
				constexpr int counts[] = {-1, -1, 32, 21, 16, 14, 13, 12, 11, 11, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 8>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 63, 40, 32, 28, 25, 23, 21, 20, 19, 19, 18, 18, 17, 17, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13};
				return counts[base];
			}

			static constexpr int max_impl(int base, std::integral_constant<std::size_t, 8>, std::false_type)
			{
				constexpr int counts[] = {-1, -1, 64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13};
				return counts[base];
			}

			static constexpr int min_impl(int base, std::integral_constant<std::size_t, 1>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 8, 5, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
				return counts[base];
			}

			static constexpr int min_impl(int /*base*/, std::integral_constant<std::size_t, 1>, std::false_type)
			{
				return 1;
			}

			static constexpr int min_impl(int base, std::integral_constant<std::size_t, 2>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 16, 10, 8, 7, 6, 6, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3};
				return counts[base];
			}

			static constexpr int min_impl(int /*base*/, std::integral_constant<std::size_t, 2>, std::false_type)
			{
				return 1;
			}

			static constexpr int min_impl(int base, std::integral_constant<std::size_t, 4>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 32, 20, 16, 14, 12, 12, 11, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6};
				return counts[base];
			}

			static constexpr int min_impl(int /*base*/, std::integral_constant<std::size_t, 4>, std::false_type)
			{
				return 1;
			}

			static constexpr int min_impl(int base, std::integral_constant<std::size_t, 8>, std::true_type)
			{
				constexpr int counts[] = {-1, -1, 64, 40, 32, 28, 25, 23, 22, 20, 19, 19, 18, 18, 17, 17, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13};
				return counts[base];
			}

			static constexpr int min_impl(int /*base*/, std::integral_constant<std::size_t, 8>, std::false_type)
			{
				return 1;
			}
		};

		template <typename T, typename Pattern>
		from_chars_result from_chars_unsigned_pattern(const char * begin, const char * end, T & value, Pattern pattern)
		{
			end = pattern.match(begin, end);
			if (begin == end)
				return {end, std::errc::invalid_argument};

			const char * const last_safe_digit = end - std::min(end - begin, detail::digits<T>::max(pattern.base) - 1);
			assert(last_safe_digit != end);

			const char * digit = end - 1;
			T x = static_cast<T>(pattern.value(*digit));
			T magnitude = static_cast<T>(pattern.base);
			while (digit != last_safe_digit)
			{
				digit--;

				x += pattern.value(*digit) * magnitude;
				magnitude *= pattern.base;
			}

			if (digit != begin)
			{
				digit--;

				T term;
				if (__builtin_mul_overflow(static_cast<T>(pattern.value(*digit)), magnitude, &term))
					return {end, std::errc::result_out_of_range};

				if (__builtin_add_overflow(x, term, &x))
					return {end, std::errc::result_out_of_range};

				for (; digit != begin;)
				{
					digit--;

					if (*digit != '0')
						return {end, std::errc::result_out_of_range};
				}
			}

			value = x;

			return {end, std::errc{}};
		}

		template <typename T, typename Pattern>
		from_chars_result from_chars_signed_pattern(const char * begin, const char * end, T & value, Pattern pattern)
		{
			end = pattern.match(begin, end);
			if (begin == end)
				return {end, std::errc::invalid_argument};

			const char * const last_safe_digit = end - std::min(end - begin, detail::digits<T>::min(pattern.base) - 1);
			assert(last_safe_digit != end);

			const char * digit = end - 1;
			T x = static_cast<T>(-pattern.value(*digit));
			T magnitude = static_cast<T>(-pattern.base);
			while (digit != last_safe_digit)
			{
				digit--;

				x += pattern.value(*digit) * magnitude;
				magnitude *= pattern.base;
			}

			if (digit != begin)
			{
				digit--;

				T term;
				if (__builtin_mul_overflow(static_cast<T>(pattern.value(*digit)), magnitude, &term))
					return {end, std::errc::result_out_of_range};

				if (__builtin_add_overflow(x, term, &x))
					return {end, std::errc::result_out_of_range};

				for (; digit != begin;)
				{
					digit--;

					if (*digit != '0')
						return {end, std::errc::result_out_of_range};
				}
			}

			value = x;

			return {end, std::errc{}};
		}

		template <typename T>
		from_chars_result from_chars_unsigned(const char * begin, const char * end, T & value, int base)
		{
			if (base <= 10)
				return detail::from_chars_unsigned_pattern(begin, end, value, detail::integer_pattern_base10{base});
			else
				return detail::from_chars_unsigned_pattern(begin, end, value, detail::integer_pattern_base36{base});
		}

		template <typename T>
		from_chars_result from_chars_signed(const char * begin, const char * end, T & value, int base)
		{
			if (begin == end)
				return {end, std::errc::invalid_argument};

			if (*begin == '-')
			{
				if (base <= 10)
					return detail::from_chars_signed_pattern(begin + 1, end, value, detail::integer_pattern_base10{base});
				else
					return detail::from_chars_signed_pattern(begin + 1, end, value, detail::integer_pattern_base36{base});
			}
			else
				return from_chars_unsigned(begin, end, value, base);
		}
	}

	inline from_chars_result from_chars(const char * begin, const char * end, signed char & value, int base = 10)
	{
		return detail::from_chars_signed(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, short int & value, int base = 10)
	{
		return detail::from_chars_signed(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, int & value, int base = 10)
	{
		return detail::from_chars_signed(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, long int & value, int base = 10)
	{
		return detail::from_chars_signed(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, long long int & value, int base = 10)
	{
		return detail::from_chars_signed(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, unsigned char & value, int base = 10)
	{
		return detail::from_chars_unsigned(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, unsigned short int & value, int base = 10)
	{
		return detail::from_chars_unsigned(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, unsigned int & value, int base = 10)
	{
		return detail::from_chars_unsigned(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, unsigned long int & value, int base = 10)
	{
		return detail::from_chars_unsigned(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, unsigned long long int & value, int base = 10)
	{
		return detail::from_chars_unsigned(begin, end, value, base);
	}

	inline from_chars_result from_chars(const char * begin, const char * end, bool & value, int base = 10)
	{
		if (base <= 10)
		{
			end = detail::integer_pattern_base10{base}.match(begin, end);
		}
		else
		{
			end = detail::integer_pattern_base36{base}.match(begin, end);
		}
		if (begin == end)
			return {end, std::errc::invalid_argument};

		while (*begin == '0')
		{
			begin++;

			if (begin == end)
			{
				value = false;

				return {end, std::errc{}};
			}
		}

		if (*begin != '1')
			return {end, std::errc::result_out_of_range};

		begin++;

		if (begin != end)
			return {end, std::errc::result_out_of_range};

		value = true;

		return {end, std::errc{}};
	}
}
