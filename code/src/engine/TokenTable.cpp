#if defined(_DEBUG) || !defined(NDEBUG)

#include "core/debug.hpp"

#include "utility/ext/stddef.hpp"
#include "utility/spinlock.hpp"

#include "ful/cstr.hpp"
#include "ful/string_modify.hpp"
#include "ful/string_init.hpp"

#include <mutex>

namespace
{
	struct range
	{
		ext::usize from;
		ext::usize count;
	};

	class impl
	{
		static constexpr ext::usize max = 100; // arbitrary

		utility::spinlock lock;
		ext::usize size = 0;

		range ranges[max];
		ful::view_utf8 names[max];
		ful::unit_utf8 * (* callbacks[max])(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end);

		ext::index find_by_range(ext::usize from, ext::usize count) const
		{
			ext::usize index = 0;
			if (index < size)
			{
				do
				{
					if (ranges[index].from == from && ranges[index].count == count)
						return index;

					index++;
				}
				while (index < size);
			}
			return -1;
		}

		ext::index find_by_value(ext::usize value) const
		{
			ext::usize index = 0;
			if (index < size)
			{
				do
				{
					if (value - ranges[index].from < ranges[index].count)
						return index;

					index++;
				}
				while (index < size);
			}
			return -1;
		}

	public:

		static impl & instance()
		{
			static impl x; // todo
			return x;
		}

		void unregister_callback(ext::usize from, ext::usize count)
		{
			std::lock_guard<utility::spinlock> guard(lock);

			const ext::index index = find_by_range(from, count);
			if (debug_assert(index >= 0))
			{
				size--;
				ranges[index] = ranges[size];
				names[index] = names[size];
				callbacks[index] = callbacks[size];
			}
		}

		bool register_callback(ext::usize from, ext::usize count, ful::view_utf8 name, ful::unit_utf8 * (* callback)(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end))
		{
			std::lock_guard<utility::spinlock> guard(lock);

			if (!debug_assert(size < max) ||
			    !debug_assert(find_by_range(from, count) < 0))
				return false;

			ranges[size].from = from;
			ranges[size].count = count;
			names[size] = name;
			callbacks[size] = callback;
			size++;

			return true;
		}

		ful::unit_utf8 * copy_string(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end)
		{
			std::lock_guard<utility::spinlock> guard(lock);

			const ext::index index = find_by_value(value);
			if (debug_assert(index >= 0))
			{
				if (callbacks[index])
				{
					return callbacks[index](value, begin, end);
				}
				else
				{
					*begin = '[';
					end = ful::copy(names[index], begin + 1, end - 1);
					*end = ']';
					return end + 1;
				}
			}
			else
			{
				return ful::copy(ful::cstr_utf8("[ ? ]"), begin, end);
			}
		}
	};
}

namespace engine
{
	void debug_tokentable_unregister(ext::usize from, ext::usize count)
	{
		return impl::instance().unregister_callback(from, count);
	}

	bool debug_tokentable_register(ext::usize from, ext::usize count, ful::view_utf8 name)
	{
		return impl::instance().register_callback(from, count, name, nullptr);
	}

	bool debug_tokentable_register(ext::usize from, ext::usize count, ful::view_utf8 name, ful::unit_utf8 * (* callback)(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end))
	{
		return impl::instance().register_callback(from, count, name, callback);
	}

	ful::unit_utf8 * debug_tokentable_copy(ext::usize value, ful::unit_utf8 * begin, ful::unit_utf8 * end)
	{
		return impl::instance().copy_string(value, begin, end);
	}
}

#endif
