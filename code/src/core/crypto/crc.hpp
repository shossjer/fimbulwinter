
#ifndef CORE_CRYPTO_CRC_HPP
#define CORE_CRYPTO_CRC_HPP

#include <utility/type_traits.hpp>

#include <array>

namespace core
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

			inline constexpr uint32_t crc32_append(const uint32_t crc, const char *const str, const std::size_t n)
			{
				return n > 0 ? crc32_append(crc32_table::values[(*str ^ crc) & 0xff] ^ (crc >> 8), str + 1, n - 1) : crc;
			}
		}

		template <std::size_t N>
		inline constexpr uint32_t crc32(const char (&str)[N])
		{
			return ~detail::crc32_append(~0, str, N - 1);
		}
		inline constexpr uint32_t crc32(const char *const str, const std::size_t n)
		{
			return ~detail::crc32_append(~0, str, n);
		}
	}
}

#endif /* CORE_CRYPTO_CRC_HPP */
