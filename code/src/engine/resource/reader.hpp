
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

			using FormatMask = uint32_t;

			constexpr FormatMask ArmatureFormat = FormatMask(1) << utility::variant_index_of<core::ArmatureStructurer, Structurer>::value;
			constexpr FormatMask JsonFormat = FormatMask(1) << utility::variant_index_of<core::JsonStructurer, Structurer>::value;
			constexpr FormatMask LevelFormat = FormatMask(1) << utility::variant_index_of<core::LevelStructurer, Structurer>::value;
			constexpr FormatMask PlaceholderFormat = FormatMask(1) << utility::variant_index_of<core::PlaceholderStructurer, Structurer>::value;
			constexpr FormatMask PngFormat = FormatMask(1) << utility::variant_index_of<core::PngStructurer, Structurer>::value;
			constexpr FormatMask ShaderFormat = FormatMask(1) << utility::variant_index_of<core::ShaderStructurer, Structurer>::value;

			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer));
			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer), FormatMask formats);
		}
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
