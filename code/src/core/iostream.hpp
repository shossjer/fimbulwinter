#pragma once

#include "fio/stdio.hpp"
#include "ful/heap.hpp"

namespace core
{
	extern thread_local fio::stdostream<ful::heap_string_utf8> cout;

#if defined(_MSC_VER)
	extern thread_local fio::stdostream<ful::heap_string_utfw> wcout;
#endif
}
