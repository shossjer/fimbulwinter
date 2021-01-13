#include "config.h"

#if MODE_DEBUG

#include "core/debug.hpp"

#include "engine/Hash.hpp"
#include "engine/HashTable.hpp"

#include "utility/spinlock.hpp"
#include "utility/unicode/string.hpp"

#include <mutex>
#include <unordered_map>

namespace
{
	class impl
	{
		utility::spinlock lock;
		std::unordered_map<engine::Hash::value_type, utility::heap_string_utf8> table;

		impl()
		{
			table.emplace(engine::Hash{}.value(), "(\"\")");
		}

		bool add_string(engine::Hash hash, utility::heap_string_utf8 && string)
		{
			std::lock_guard<utility::spinlock> guard(lock);
			const auto p = table.emplace(hash.value(), std::move(string));
			return p.second || p.first->second == string;
		}

	public:

		static impl & instance()
		{
			static impl x; // todo
			return x;
		}

		bool add_string(engine::Hash hash, const char * str)
		{
			utility::heap_string_utf8 string;
			if (!(debug_verify(string.try_append("(\"")) &&
			      debug_verify(string.try_append(str)) &&
			      debug_verify(string.try_append("\")"))))
				return false;

			return debug_verify(add_string(hash, std::move(string)));
		}

		bool add_string(engine::Hash hash, const char * str, std::size_t n)
		{
			utility::heap_string_utf8 string;
			if (!(debug_verify(string.try_append("(\"")) &&
			      debug_verify(string.try_append(str, n)) &&
			      debug_verify(string.try_append("\")"))))
				return false;

			return debug_verify(add_string(hash, std::move(string)));
		}

		utility::string_units_utf8 lookup_string(engine::Hash hash)
		{
			std::lock_guard<utility::spinlock> guard(lock);
			const auto it = table.find(hash.value());
			return it != table.end() ? utility::string_units_utf8(it->second) : utility::string_units_utf8("( ? )");
		}
	};
}

namespace engine
{
	HashTable::HashTable(std::initializer_list<const char *> strs)
	{
		utility::heap_string_utf8 message;
		message.try_append("adding hashes:");
		for (const char * str : strs)
		{
			message.try_append(" \"");
			message.try_append(str);
			message.try_append("\"");
		}
		debug_printline(message);

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

	std::ostream & operator << (std::ostream & stream, const Hash & hash)
	{
		// note we copy just in case someone alters the hash while we
		// are printing it so that the value and name guarantees* to
		// match

		// *) guarantee is a bit misleading because Hash is not thread
		// safe so all bets are off if someone alters it but this
		// function is meant for debugging purposes, so something that
		// "might" do the right thing is better than something that does
		// not even try, maybe
		const auto cpy = hash;
		// todo take some time and look at the assembly to see if there
		// actually is any difference whatsoever, maybe the compiler is
		// smart enough

		return stream << cpy.value() << impl::instance().lookup_string(cpy);
	}
}

#endif
