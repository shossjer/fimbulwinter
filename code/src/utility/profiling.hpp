#pragma once

#if PROFILING_COZ
# include <coz.h>
#endif

namespace utility
{
	namespace detail
	{
#if PROFILING_COZ
		struct CallWhenDestroyed
		{
			void (* func)();

			~CallWhenDestroyed()
			{
				func();
			}

			explicit CallWhenDestroyed(void (* func)())
				: func(func)
			{}
		};
#endif
	}
}

#if PROFILING_COZ
# define profile_begin(name) do {} while(0)
# define profile_end(name) COZ_PROGRESS_NAMED(name)

# define profile_scope(name) utility::detail::CallWhenDestroyed _profile_scope_([](){ COZ_PROGRESS_NAMED(name); });
#else
# define profile_begin(name) do {} while(0)
# define profile_end(name) do {} while(0)

# define profile_scope(name) do {} while(0)
#endif
