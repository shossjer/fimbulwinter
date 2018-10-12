
#ifndef ENGINE_RESOURCE_WRITER_HPP
#define ENGINE_RESOURCE_WRITER_HPP

#include "core/IniSerializer.hpp"
#include "core/JsonSerializer.hpp"

#include "engine/resource/formats.hpp"

#include "utility/variant.hpp"

#include <string>

namespace engine
{
	namespace resource
	{
		namespace writer
		{
			using Serializer = utility::variant<
				core::IniSerializer,
				core::JsonSerializer
			>;

			void post_write(std::string name, void (* callback)(std::string name, Serializer & serializer));
		}
	}
}

#endif /* ENGINE_RESOURCE_WRITER_HPP */
