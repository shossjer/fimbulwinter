#pragma once

#include "config.h"

#if MODE_DEBUG
# include <ostream>
#endif

namespace engine
{
	class CookieFactory;

	class Cookie
	{
		friend CookieFactory;

		using this_type = Cookie;
		using value_type = long long;

	private:

		value_type id_;

	private:

		friend bool operator == (this_type a, this_type b) { return a.id_ == b.id_; }
		friend bool operator != (this_type a, this_type b) { return !(a == b); }

#if MODE_DEBUG
		friend std::ostream & operator << (std::ostream & stream, this_type x)
		{
			return stream << x.id_;
		}
#endif
	};
}
