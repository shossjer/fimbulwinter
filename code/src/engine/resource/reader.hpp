
#ifndef ENGINE_RESOURCE_READER_HPP
#define ENGINE_RESOURCE_READER_HPP

#include "core/ArmatureStructurer.hpp"
#include "core/IniStructurer.hpp"
#include "core/JsonStructurer.hpp"
#include "core/LevelStructurer.hpp"
#include "core/PlaceholderStructurer.hpp"
#include "core/PngStructurer.hpp"
#include "core/ShaderStructurer.hpp"

#include "engine/resource/formats.hpp"

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
				core::IniStructurer,
				core::JsonStructurer,
				core::LevelStructurer,
				core::PlaceholderStructurer,
				core::PngStructurer,
				core::ShaderStructurer
				>;

			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer), FormatMask formats);

			inline void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer))
			{
				return post_read(std::move(name), callback, FormatMask::all());
			}
		}
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
