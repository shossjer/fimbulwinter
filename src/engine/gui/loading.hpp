
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_LOADING_HPP
#define ENGINE_GUI_LOADING_HPP

#include "views.hpp"
#include "functions.hpp"

#include <engine/common.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
namespace gui
{
	struct DataGroup;
	struct DataPanel;
	struct DataText;
	struct DataTexture;

	using DataVariant = utility::variant
	<
		DataGroup,
		DataPanel,
		DataText,
		DataTexture
	>;

	struct DataView
	{
		engine::Asset name;
		View::Size size;
		View::Margin margin;
		View::Gravity gravity;

		struct Action
		{
			constexpr static engine::Asset CLOSE{ "close" };
			constexpr static engine::Asset MOVER{ "mover" };
			constexpr static engine::Asset SELECT{ "selectable" };

			engine::Asset type;
		}
		action;

		struct Function
		{
			constexpr static engine::Asset PROGRESS{ "progressBar" };

			engine::Asset type;
			engine::Asset name;
			engine::gui::ProgressBar::Direction direction;
		}
		function;

		DataView(engine::Asset name, View::Size size, View::Margin margin, View::Gravity gravity)
			: name(name)
			, size(size)
			, margin(margin)
			, gravity(gravity)
		{
			action.type = engine::Asset::null();
			function.type = engine::Asset::null();
		}

		bool has_action() const
		{
			return this->action.type!= engine::Asset::null();
		}

		bool has_function() const
		{
			return this->function.type != engine::Asset::null();
		}

		bool has_name() const
		{
			return this->name != engine::Asset::null();
		}
	};

	struct DataGroup : DataView
	{
		Group::Layout layout;
		std::vector<DataVariant> children;

		DataGroup(engine::Asset name, View::Size size, View::Margin margin, View::Gravity gravity, Group::Layout layout)
			: DataView(name, size, margin, gravity)
			, layout(layout)
		{}
	};

	struct DataPanel : DataView
	{
		Color color;

		DataPanel(engine::Asset name, View::Size size, View::Margin margin, View::Gravity gravity, Color color)
			: DataView(name, size, margin, gravity)
			, color(color)
		{}
	};

	struct DataText : DataView
	{
		Color color;
		std::string display;

		DataText(engine::Asset name, View::Size size, View::Margin margin, View::Gravity gravity, Color color, std::string display)
			: DataView(name, size, margin, gravity)
			, color(color)
			, display(display)
		{}
	};

	struct DataTexture : DataView
	{
		engine::Asset res;

		DataTexture(engine::Asset name, View::Size size, View::Margin margin, View::Gravity gravity, engine::Asset res)
			: DataView(name, size, margin, gravity)
			, res(res)
		{}
	};
}
}

#endif // ENGINE_GUI_LOADING_HPP
