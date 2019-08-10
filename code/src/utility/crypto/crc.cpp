
#include "crc.hpp"

namespace core
{
	namespace crypto
	{
		namespace detail
		{
			constexpr std::array<uint32_t, 256> crc32_table::values;
		}
	}
}
