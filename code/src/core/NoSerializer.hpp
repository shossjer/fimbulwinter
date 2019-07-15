
#ifndef CORE_NOSERIALIZER_HPP
#define CORE_NOSERIALIZER_HPP

#include <string>

namespace core
{
	struct NoSerializer
	{
		std::string filename;

		NoSerializer(std::string filename)
			: filename(std::move(filename))
		{}
	};
}

#endif /* CORE_NOSERIALIZER_HPP */
