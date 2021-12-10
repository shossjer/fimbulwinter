#pragma once

#include "config.h"

#if MODE_DEBUG

#include "ful/cstr.hpp"

#include <cstdint>
#include <initializer_list>

namespace engine
{
	class Hash;

	struct HashTable
	{
		HashTable() = default;

		explicit HashTable(std::initializer_list<const char *> strs);

		void add(const Hash & hash, const char * str);
		void add(const Hash & hash, const char * str, std::size_t n);
	};
}

#define static_hashes(...) static engine::HashTable add_static_hashes({__VA_ARGS__})

#else

#define static_hashes(...) static_assert(true, "")

#endif
