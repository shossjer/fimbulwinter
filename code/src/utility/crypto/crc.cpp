
#include "crc.hpp"

namespace utility
{
	namespace crypto
	{
		namespace detail
		{
			constexpr std::array<uint32_t, 256> crc32_table::values;
		}
	}
}
