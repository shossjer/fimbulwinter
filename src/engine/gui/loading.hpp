
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_LOADING_HPP
#define ENGINE_GUI_LOADING_HPP

#include "functions.hpp"
#include "placers.hpp"

#include <engine/common.hpp>
#include <engine/graphics/renderer.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
namespace gui
{
	typedef engine::graphics::data::Color Color;

	struct GroupData;
	struct ListData;
	struct PanelData;
	struct TextData;
	struct TextureData;

	using DataVariant = utility::variant
	<
		GroupData,
		ListData,
		PanelData,
		TextData,
		TextureData
	>;

	struct ViewData
	{
		engine::Asset name;
		Size size;
		Margin margin;
		Gravity gravity;

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

		ViewData(engine::Asset name, Size size, Margin margin, Gravity gravity)
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

	struct GroupData : ViewData
	{
		Layout layout;
		std::vector<DataVariant> children;

		GroupData(engine::Asset name, Size size, Margin margin, Gravity gravity, Layout layout)
			: ViewData(name, size, margin, gravity)
			, layout(layout)
		{}
	};

	struct ListData : GroupData
	{
		ListData(engine::Asset name, Size size, Margin margin, Gravity gravity, Layout layout)
			: GroupData(name, size, margin, gravity, layout)
		{}
	};

	struct PanelData : ViewData
	{
		Color color;

		PanelData(engine::Asset name, Size size, Margin margin, Gravity gravity, Color color)
			: ViewData(name, size, margin, gravity)
			, color(color)
		{}
	};

	struct TextData : ViewData
	{
		Color color;
		std::string display;

		TextData(engine::Asset name, Size size, Margin margin, Gravity gravity, Color color, std::string display)
			: ViewData(name, size, margin, gravity)
			, color(color)
			, display(display)
		{}
	};

	struct TextureData : ViewData
	{
		engine::Asset res;

		TextureData(engine::Asset name, Size size, Margin margin, Gravity gravity, engine::Asset res)
			: ViewData(name, size, margin, gravity)
			, res(res)
		{}
	};
}
}

#endif // ENGINE_GUI_LOADING_HPP
