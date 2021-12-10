#include "config.h"

#if MODE_DEBUG

#include "core/debug.hpp"

#include "engine/Hash.hpp"
#include "engine/HashTable.hpp"

#include "utility/spinlock.hpp"

#include "ful/heap.hpp"
#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"
#include "ful/string_search.hpp"

#include <mutex>
#include <unordered_map>

namespace
{
	class impl
	{
		utility::spinlock lock;
		std::unordered_map<engine::Hash::value_type, ful::heap_string_utf8> table;

		impl()
		{
			ful::heap_string_utf8 string;
			append(string, ful::cstr_utf8("(\"\")"));
			table.emplace(engine::Hash{}.value(), std::move(string));
		}

		ful::cstr_utf8 add_string(engine::Hash hash, ful::heap_string_utf8 && string)
		{
			std::lock_guard<utility::spinlock> guard(lock);
			const auto p = table.emplace(hash.value(), std::move(string));
			return ful::cstr_utf8(p.first->second);
		}

	public:

		static impl & instance()
		{
			static impl x; // todo
			return x;
		}

		bool add_string(engine::Hash hash, const char * str)
		{
			ful::heap_string_utf8 string;
			if (!(debug_verify(ful::append(string, ful::cstr_utf8("(\""))) &&
			      debug_verify(ful::append(string, str, ful::strend(str))) &&
			      debug_verify(ful::append(string, ful::cstr_utf8("\")")))))
				return false;

			const auto found_string = add_string(hash, std::move(string));
			return debug_verify(ful::view_utf8(found_string.begin() + 2, found_string.end() - 2) == str);
		}

		bool add_string(engine::Hash hash, const char * str, std::size_t n)
		{
			ful::heap_string_utf8 string;
			if (!(debug_verify(ful::append(string, ful::cstr_utf8("(\""))) &&
			      debug_verify(ful::append(string, str, str + n)) &&
			      debug_verify(ful::append(string, ful::cstr_utf8("\")")))))
				return false;

			const auto found_string = add_string(hash, std::move(string));
			return debug_verify(ful::view_utf8(found_string.begin() + 2, found_string.end() - 2) == ful::view_utf8(str, n));
		}

		ful::cstr_utf8 lookup_string(engine::Hash hash)
		{
			std::lock_guard<utility::spinlock> guard(lock);
			const auto it = table.find(hash.value());
			return it != table.end() ? ful::cstr_utf8(it->second) : ful::cstr_utf8("( ? )");
		}
	};
}

namespace engine
{
	ful::view_utf8 Hash::get_string_from_hash_table(Hash hash)
	{
		return impl::instance().lookup_string(hash);
	}

	HashTable::HashTable(std::initializer_list<const char *> strs)
	{
		for (const char * str : strs)
		{
			// todo locking is over-restrictive since this constructor
			// should only be called before main, which is guaranteed to
			// be single threaded <insert standardese here>
			impl::instance().add_string(engine::Hash(str), str);
		}
	}

	void HashTable::add(const Hash & hash, const char * str)
	{
		impl::instance().add_string(hash, str);
	}

	void HashTable::add(const Hash & hash, const char * str, std::size_t n)
	{
		impl::instance().add_string(hash, str, n);
	}
}

#endif
