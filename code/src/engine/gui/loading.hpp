
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_LOADING_HPP
#define ENGINE_GUI_LOADING_HPP

#include "common.hpp"
#include "function.hpp"
#include "view.hpp"

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

		struct Action
		{
			static constexpr Asset CLOSE{ "close" };
			static constexpr Asset INTERACTION{ "interaction" };
			static constexpr Asset MOVER{ "mover" };
			static constexpr Asset SELECT{ "selectable" };
			static constexpr Asset TRIGGER{ "trigger" };

			Asset type;
			Asset target;
		};

		std::vector<Action> actions;

		struct Function
		{
			static constexpr Asset LIST{ "list" };
			static constexpr Asset PROGRESS{ "progressBar" };
			static constexpr Asset TAB{ "tabBar" };

			Asset type;
			std::string name;
			std::vector<DataVariant> templates;
			//GroupData group_template;
		}
		function;

		ViewData(std::string name, Size size, Margin margin, Gravity gravity)
			: name(name)
			, size(size)
			, margin(margin)
			, gravity(gravity)
		{
			function.type = Asset::null();
		}

		bool has_action() const
		{
			return !this->actions.empty();
		}

		bool has_name() const
		{
			return !this->name.empty();
		}

		// in one place a dynamic cast is performed...
		virtual void dummy() {};
	};

	struct GroupData : ViewData
	{
		View::Group::Layout layout;
		std::vector<DataVariant> children;

		GroupData(std::string name, Size size, Margin margin, Gravity gravity, View::Group::Layout layout)
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
}
}

#endif // ENGINE_GUI_LOADING_HPP
