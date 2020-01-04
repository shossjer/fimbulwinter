
#ifndef UTILITY_CRYPTO_CRC_HPP
#define UTILITY_CRYPTO_CRC_HPP

#include "utility/type_traits.hpp"

#include <array>

namespace utility
{
	namespace crypto
	{
		namespace detail
		{
			inline constexpr uint32_t crc32_table_gen(const uint32_t rem, const std::size_t n = 8)
			{
				return n > 0 ? crc32_table_gen((rem & 1 ? 0xedb88320 : 0) ^ (rem >> 1), n - 1) : rem;
			}

			template <typename>
			struct crc32_table_impl;
			template <std::size_t ...Is>
			struct crc32_table_impl<mpl::index_sequence<Is...>>
			{
				static constexpr std::array<uint32_t, sizeof...(Is)> values{{crc32_table_gen(Is)...}};
			};

			struct crc32_table
			{
				static constexpr std::array<uint32_t, 256> values = crc32_table_impl<mpl::make_index_sequence<256>>::values;
			};

			inline constexpr uint32_t crc32_accumulate(const uint32_t crc, const char c)
			{
				return crc32_table::values[(uint32_t(c) ^ crc) & 0xff] ^ (crc >> 8);
			}

			inline constexpr uint32_t crc32_append(const uint32_t crc, const char *const str, const std::size_t n)
			{
				return n <= 0 ? crc : crc32_append(crc32_accumulate(crc, *str), str + 1, n - 1);
			}
		}

		template <std::size_t N>
		inline constexpr uint32_t crc32(const char (& str)[N])
		{
			return ~detail::crc32_append(~0u, str, N - 1);
		}
		inline constexpr uint32_t crc32(const char * const str, const std::size_t n)
		{
			return ~detail::crc32_append(~0u, str, n);
		}
	}
}

#endif /* UTILITY_CRYPTO_CRC_HPP */
