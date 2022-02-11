#pragma once

#include "config.h"

#include "utility/ext/stddef.hpp"

#include "ful/view.hpp"

namespace engine
{
#if MODE_DEBUG
	extern void debug_tokentable_unregister(ext::usize from, ext::usize count);
	extern bool debug_tokentable_register(ext::usize from, ext::usize count, ful::view_utf8 name);
#endif

	class EntityFactory
	{
	public:

		ext::usize from;
		ext::usize count;

		~EntityFactory()
		{
#if MODE_DEBUG
			if (count != 0)
			{
				debug_tokentable_unregister(from, count);
			}
#endif
		}

		explicit EntityFactory(ful::view_utf8 name) noexcept
			: EntityFactory(~(ext::usize(-1) >> 1), ~(ext::usize(-1) >> 1), name)
		{}

		explicit EntityFactory(ext::usize from, ext::usize count, ful::view_utf8 name) noexcept
			: from(from)
			, count(count)
		{
#if MODE_DEBUG
			if (count != 0)
			{
				if (!debug_tokentable_register(from, count, name))
				{
					// disable this factory so the destructor works as intended
					count = 0;
				}
			}
#endif
		}

		EntityFactory(EntityFactory && other) noexcept
			: from(other.from)
			, count(other.count)
		{
#if MODE_DEBUG
			other.count = 0;
#endif
		}

		EntityFactory & operator =(EntityFactory && other) noexcept
		{
#if MODE_DEBUG
			if (count != 0)
			{
				debug_tokentable_unregister(from, count);
			}
#endif

			from = other.from;
			count = other.count;

#if MODE_DEBUG
			other.count = 0;
#endif

			return *this;
		}

	};
}
