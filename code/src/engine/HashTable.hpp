#pragma once

#if defined(_DEBUG) || !defined(NDEBUG)

#include "utility/compiler.hpp"
#include "utility/crypto/crc.hpp"

#include "ful/cstr.hpp"

namespace engine
{
	extern void debug_hashtable_add(std::uint32_t value, ful::view_utf8 string);

	struct StaticHashes
	{
		template <typename ...Ps>
		explicit StaticHashes(Ps && ...ps)
		{
			int expansion_hack[] = {(debug_hashtable_add(utility::crypto::crc32(ps), ful::cstr_utf8(ps)), 0)...};
			fiw_unused(expansion_hack);
		}
	};
}

#define static_hashes(...) static engine::StaticHashes __static_hashes__(__VA_ARGS__)

#else

#define static_hashes(...) static_assert(true, "")

#endif
