
#ifndef ENGINE_RESOURCE_READER_HPP
#define ENGINE_RESOURCE_READER_HPP

#include "core/ArmatureStructurer.hpp"
#include "core/JsonStructurer.hpp"
#include "core/LevelStructurer.hpp"
#include "core/PlaceholderStructurer.hpp"
#include "core/PngStructurer.hpp"
#include "core/ShaderStructurer.hpp"

#include "utility/variant.hpp"

#include <string>

namespace engine
{
	namespace resource
	{
		namespace reader
		{
			using Structurer = utility::variant<
				core::ArmatureStructurer,
				core::JsonStructurer,
				core::LevelStructurer,
				core::PlaceholderStructurer,
				core::PngStructurer,
				core::ShaderStructurer
				>;

			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer));
		}
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
