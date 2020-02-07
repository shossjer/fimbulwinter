
#ifndef CORE_NOSERIALIZER_HPP
#define CORE_NOSERIALIZER_HPP

#include "utility/unicode.hpp"

namespace core
{
	struct NoSerializer
	{
		utility::heap_string_utf8 filename;

		NoSerializer(utility::heap_string_utf8 && filename)
			: filename(std::move(filename))
		{}
	};
}

#endif /* CORE_NOSERIALIZER_HPP */
