
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

		ViewData(std::string name, Size size, Margin margin, Gravity gravity)
			: name(name)
			, size(size)
			, margin(margin)
			, gravity(gravity)
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

		GroupData(std::string name, Size size, Margin margin, Gravity gravity, Layout layout)
			: ViewData(name, size, margin, gravity)
			, layout(layout)
		{}
	};

	struct PanelData : ViewData
	{
		Asset color;

		PanelData(std::string name, Size size, Margin margin, Gravity gravity, Asset color)
			: ViewData(name, size, margin, gravity)
			, color(color)
		{}
	};

	struct TextData : ViewData
	{
		Asset color;
		std::string display;

		TextData(std::string name, Size size, Margin margin, Gravity gravity, Asset color, std::string display)
			: ViewData(name, size, margin, gravity)
			, color(color)
			, display(display)
		{}
	};

	struct TextureData : ViewData
	{
		Asset texture;

		TextureData(std::string name, Size size, Margin margin, Gravity gravity, Asset texture)
			: ViewData(name, size, margin, gravity)
			, texture(texture)
		{}
	};

	std::vector<DataVariant> load();
}
}

#endif // ENGINE_GUI_VIEW_CREATION_HPP
