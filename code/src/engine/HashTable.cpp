#if defined(_DEBUG) || !defined(NDEBUG)

#include "core/debug.hpp"

#include "utility/ext/stddef.hpp"
#include "utility/spinlock.hpp"

#include "ful/heap.hpp"
#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"

#include <mutex>
#include <unordered_map>

namespace engine
{
	extern void debug_tokentable_unregister(ext::usize from, ext::usize count);
	extern bool debug_tokentable_register(ext::usize from, ext::usize count, ful::view_utf8 name, ful::unit_utf8 * (* callback)(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end));

	ful::unit_utf8 * debug_hashtable_copy(std::uint32_t value, ful::unit_utf8 * begin, ful::unit_utf8 * end);
}

namespace
{
	ful::unit_utf8 * hashtable_copy(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end)
	{
		if (debug_assert(value <= std::uint32_t(-1)))
		{
			return engine::debug_hashtable_copy(static_cast<std::uint32_t>(value), begin, end);
		}
		else
		{
			return ful::copy(ful::cstr_utf8("( ? )"), begin, end);
		}
	}

	class impl
	{
		utility::spinlock lock;
		std::unordered_map<std::uint32_t, ful::heap_string_utf8> table;

		~impl()
		{
			engine::debug_tokentable_unregister(0, std::uint32_t(-1) & (ext::usize(-1) >> 1));
		}

		impl()
		{
			engine::debug_tokentable_register(0, std::uint32_t(-1) & (ext::usize(-1) >> 1), ful::cstr_utf8("__hash_table__"), hashtable_copy);

			ful::heap_string_utf8 string;
			table.emplace(std::uint32_t{}, std::move(string));
		}

	public:

		static impl & instance()
		{
			static impl x; // todo
			return x;
		}

		bool add_string(std::uint32_t value, ful::view_utf8 string)
		{
			ful::heap_string_utf8 copy;
			if (!debug_verify(ful::append(copy, string)))
				return false;

			{
				std::lock_guard<utility::spinlock> guard(lock);

				const auto p = table.emplace(value, std::move(copy));
				return p.first != table.end();
			}
		}

		ful::unit_utf8 * copy_string(std::uint32_t value, ful::unit_utf8 * begin, ful::unit_utf8 * end)
		{
			std::lock_guard<utility::spinlock> guard(lock);
			const auto it = table.find(value);
			if (debug_assert(it != table.end()))
			{
				*begin = '(';
				end = ful::copy(it->second, begin + 1, end - 1);
				*end = ')';
				return end + 1;
			}
			else
			{
				return ful::copy(ful::cstr_utf8("( ? )"), begin, end);
			}
		}
	};
}

namespace engine
{
	ful::unit_utf8 * debug_hashtable_copy(std::uint32_t value, ful::unit_utf8 * begin, ful::unit_utf8 * end)
	{
		return impl::instance().copy_string(value, begin, end);
	}

	void debug_hashtable_add(std::uint32_t value, ful::view_utf8 string)
	{
		impl::instance().add_string(value, string);
	}
}

#endif
