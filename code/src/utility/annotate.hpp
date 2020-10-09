#pragma once

#if defined(_MSC_VER)
# define annotate_nodiscard _Check_return_
#else
# define annotate_nodiscard __attribute__((__warn_unused_result__))
#endif
