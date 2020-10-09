#pragma once

// todo forward declare
#include "utility/unicode/string.hpp"

namespace core
{
	class ReadStream;
}

namespace core
{
	namespace native
	{
		bool try_read_file(utility::heap_string_utf8 && filepath, void (* callback)(core::ReadStream && stream, void * data), void * data);
	}
}
