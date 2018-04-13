
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_VIEW_CREATION_HPP
#define ENGINE_GUI_VIEW_CREATION_HPP

// this dependency could be removed if margin, gravity etc is replaced by some "load" version
#include "view_data.hpp"

#include <engine/Asset.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
namespace gui
{
	struct GroupData;
	struct PanelData;
	struct TextData;
	struct TextureData;

	using DataVariant = utility::variant
	<
		GroupData,
		PanelData,
		TextData,
		TextureData
	>;

	struct ViewData
	{
		std::string name;
		Size size;
		Margin margin;
		Gravity gravity;

	//	std::vector<InteractionData> interactions;
	//	std::vector<ReactionData> reactions;

		ViewData(Size size)
			: name()
			, size(size)
			, margin()
			, gravity()
		{
		}

		bool has_name() const
		{
			return !this->name.empty();
		}
	};

	struct GroupData : ViewData
	{
        Layout layout;
		std::vector<DataVariant> children;

		GroupData()
			: ViewData(Size{ Size::PARENT, Size::PARENT })
			, layout()
		{}
	};

	struct PanelData : ViewData
	{
		Asset color;

		PanelData()
			: ViewData(Size{ Size::PARENT, Size::PARENT })
			, color(Asset::null())
		{}
	};

	struct TextData : ViewData
	{
		Asset color;
		std::string display;

		TextData()
			: ViewData(Size{ Size::WRAP, Size::WRAP })
			, color(Asset::null())
			, display()
		{}
	};

	struct TextureData : ViewData
	{
		Asset texture;

		TextureData()
			: ViewData(Size{ Size::PARENT, Size::PARENT })
			, texture(Asset::null())
		{}
	};

	std::vector<DataVariant> load();
}
}

#endif // ENGINE_GUI_VIEW_CREATION_HPP
