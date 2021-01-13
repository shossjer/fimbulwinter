#include "config.h"

#if MODE_DEBUG

#include "engine/Asset.hpp"
#include "engine/HashTable.hpp"

namespace engine
{
	void Asset::add_string_to_hash_table(const char * str)
	{
		HashTable{}.add(*this, str);
	}

	void Asset::add_string_to_hash_table(const char * str, std::size_t n)
	{
		HashTable{}.add(*this, str, n);
	}
}

#endif
