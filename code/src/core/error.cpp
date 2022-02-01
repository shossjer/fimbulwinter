#include "core/content.hpp"
#include "core/debug.hpp" // todo make it the other way around, make debug depend on error
#include "core/error.hpp"

namespace
{
	void find_row_and_col(ext::ssize size, const ful::unit_utf8 * end, ext::ssize & row, ext::ssize & col)
	{
		ext::ssize m = 0;
		ext::ssize n = size;

		if (size < 0)
		{
			if (*(end + size) != '\n')
			{
				while (true)
				{
					do
					{
						size++;
						if (size >= 0)
							goto done;
					}
					while (*(end + size) != '\n');

					m++;
					n = size;
				}
			}
		}
	done:
		row = m;
		col = 1 - n;
	}
}

namespace core
{
	void report_text_error(core::content & content, const text_error & error)
	{
		ful::unit_utf8 * const begin = static_cast<ful::unit_utf8 *>(content.data());
		ful::unit_utf8 * const end = static_cast<ful::unit_utf8 *>(content.data()) + content.size();

		ext::ssize row;
		ext::ssize col;
		find_row_and_col((begin - end) - error.where, end + error.where, row, col);

#if defined(_MSC_VER)
		core::debug::instance().fail(content.filepath(), '(', row, "): ", error.message, empty(error.type) ? ful::view_utf8{} : ful::cstr_utf8(", with type "), empty(error.type) ? ful::view_utf8{} : error.type);
#else
		core::debug::instance().fail(content.filepath(), ':', row, ": ", error.message, empty(error.type) ? ful::view_utf8{} : ful::cstr_utf8(", with type "), empty(error.type) ? ful::view_utf8{} : error.type);
#endif
	}
}
