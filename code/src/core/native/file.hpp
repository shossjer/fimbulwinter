#pragma once

#include "ful/cstr.hpp"

namespace core
{
	class content;
}

namespace core
{
	namespace native
	{
		int try_read_file(ful::cstr_utf8 filepath, bool (* callback)(core::content & content, void * data), void * data);
	}
}
