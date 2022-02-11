#pragma once

#include "ful/cstr.hpp"

namespace core
{
	class ReadStream;
}

namespace core
{
	namespace native
	{
		bool try_read_file(ful::cstr_utf8 filepath, void (* callback)(core::ReadStream && stream, void * data), void * data);
	}
}
