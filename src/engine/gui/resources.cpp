
#include "resources.hpp"

#include "views.hpp"

namespace
{
	using namespace engine::gui;

	struct Simple : resource::ColorResource
	{
		Color color;

		Simple(Color color)
			: color(color)
		{}

		Color get(const View * view) const override
		{
			return this->color;
		}
	};

	struct ColorSelector : resource::ColorResource
	{
		engine::Asset state_default;
		engine::Asset state_highlight;
		engine::Asset state_pressed;

		ColorSelector(const engine::Asset def, const engine::Asset high, const engine::Asset press)
			: state_default(def)
			, state_highlight(high)
			, state_pressed(press)
		{}

		Color get(const View * view) const override;
	};

	core::container::UnorderedCollection
	<
		engine::Asset, 201,
		std::array<Simple, 100>,
		std::array<ColorSelector, 20>
	>
	resources;

	Color ColorSelector::get(const View * view) const
	{
		switch (view->state)
		{
		case View::State::HIGHLIGHT:
			return resources.get<Simple>(this->state_highlight).color;
		case View::State::PRESSED:
			return resources.get<Simple>(this->state_pressed).color;
		case View::State::DEFAULT:
		default:
			return resources.get<Simple>(this->state_default).color;
		}
	}

	struct
	{
		const resource::ColorResource * operator() (const Simple & val)
		{
			return &val;
		}

		const resource::ColorResource * operator() (const ColorSelector & val)
		{
			return &val;
		}
	}
	lookup_color;
}

namespace engine
{
namespace gui
{
	namespace resource
	{
		const ColorResource * color(const engine::Asset asset)
		{
			return resources.call(asset, lookup_color);
		}

		void put(const engine::Asset asset, const Color val)
		{
			resources.emplace<Simple>(asset, val);
		}

		void put(const engine::Asset asset, const engine::Asset def, const engine::Asset high, const engine::Asset press)
		{
			resources.emplace<ColorSelector>(asset, def, high, press);
		}
	}
}
}
