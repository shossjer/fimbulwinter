#pragma once

#include "utility/ext/stddef.hpp"

#include "ful/view.hpp"

namespace core
{
	class content;
}

namespace core
{
	struct text_error
	{
		ext::ssize where;
		ful::view_utf8 message;
		ful::view_utf8 type;
	};

	void report_text_error(core::content & content, const text_error & error);
}
