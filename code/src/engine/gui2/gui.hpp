
#ifndef ENGINE_GUI2_GUI_HPP
#define ENGINE_GUI2_GUI_HPP

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui2
	{
		struct ListData
		{
			int count;
		};
		struct TextData
		{
			std::string display;
		};

		// defined publicly in engine::gui
		using DataVariant = utility::variant
		<
			ListData,
			TextData
		>;

		using Datas = std::vector<std::pair<engine::Asset, DataVariant>>;

		void post_update(engine::Asset key, Datas && datas);
	}
}

#endif
