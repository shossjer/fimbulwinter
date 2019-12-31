
#ifndef ENGINE_RESOURCE_READER_HPP
#define ENGINE_RESOURCE_READER_HPP

#include "core/ArmatureStructurer.hpp"
#include "core/BytesStructurer.hpp"
#include "core/IniStructurer.hpp"
#include "core/JsonStructurer.hpp"
#include "core/LevelStructurer.hpp"
#include "core/PlaceholderStructurer.hpp"
#include "core/PngStructurer.hpp"
#include "core/ShaderStructurer.hpp"
#include "core/NoSerializer.hpp"

#include "engine/resource/formats.hpp"

#include "utility/variant.hpp"

#include <string>

namespace engine
{
	namespace resource
	{
		class reader
		{
		public:
			using Structurer = utility::variant<
				core::ArmatureStructurer,
				core::BytesStructurer,
				core::IniStructurer,
				core::JsonStructurer,
				core::LevelStructurer,
				core::PlaceholderStructurer,
				core::PngStructurer,
				core::ShaderStructurer,
				core::NoSerializer
				>;

		public:
			~reader();
			reader();

		public:
			void post_read(std::string name,
			               void (* callback)(std::string name, Structurer && structurer),
			               FormatMask formats);
			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer))
			{
				post_read(std::move(name), callback, FormatMask::all());
			}
		};
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
