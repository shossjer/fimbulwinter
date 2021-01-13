#pragma once

#include "utility/type_traits.hpp"

#include <array>
#include <cstdint>

namespace utility
{
	namespace crypto
	{
		namespace detail
		{
			inline constexpr std::uint32_t crc32_table_gen(std::uint32_t rem, std::size_t n = 8)
			{
				return 0 < n ? crc32_table_gen((rem & 1 ? 0xedb88320 : 0) ^ (rem >> 1), n - 1) : rem;
			}

			template <typename>
			struct crc32_table_impl;

			template <std::size_t ...Is>
			struct crc32_table_impl<mpl::index_sequence<Is...>>
			{
				static constexpr std::array<std::uint32_t, sizeof...(Is)> values{{crc32_table_gen(Is)...}};
			};

			struct crc32_table
			{
				static constexpr std::array<std::uint32_t, 256> values = crc32_table_impl<mpl::make_index_sequence<256>>::values;
			};

			inline constexpr std::uint32_t crc32_accumulate(std::uint32_t crc, char c)
			{
				return crc32_table::values[(std::uint32_t(c) ^ crc) & 0xff] ^ (crc >> 8);
			}

			inline constexpr std::uint32_t crc32_append(std::uint32_t crc, const char * str)
			{
				return *str == '\0' ? crc : crc32_append(crc32_accumulate(crc, *str), str + 1);
			}

			inline constexpr std::uint32_t crc32_append(std::uint32_t crc, const char * str, std::size_t n)
			{
				return n <= 0 ? crc : crc32_append(crc32_accumulate(crc, *str), str + 1, n - 1);
			}
		}

		inline constexpr std::uint32_t crc32(const char * str)
		{
			return ~detail::crc32_append(~0u, str);
		}

		inline constexpr std::uint32_t crc32(const char * str, std::size_t n)
		{
			return ~detail::crc32_append(~0u, str, n);
		}
	}
}
