#include "fio/stdio.hpp"
#include "ful/heap.hpp"

namespace core
{
	// todo fio_thread causes problems with non-trivial destructor
	thread_local fio::stdostream<ful::heap_string_utf8> cout;

#if defined(_MSC_VER)
	thread_local fio::stdostream<ful::heap_string_utfw> wcout;
#endif
}
