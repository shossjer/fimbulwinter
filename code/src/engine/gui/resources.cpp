
#include "resources.hpp"
#include "view.hpp"

#include <core/container/Collection.hpp>

namespace
{
	using namespace engine::gui;

	struct Simple : resource::Color
	{
		resource::ColorValue color;

		Simple(resource::ColorValue color)
			: color(color)
		{}

		resource::ColorValue get(const View * view) const override
		{
			return this->color;
		}
	};

	struct ColorSelector : resource::Color
	{
		engine::Asset state_default;
		engine::Asset state_highlight;
		engine::Asset state_pressed;

		ColorSelector(const engine::Asset def, const engine::Asset high, const engine::Asset press)
			: state_default(def)
			, state_highlight(high)
			, state_pressed(press)
		{}

		resource::ColorValue get(const View * view) const override;
	};

	using Resources = core::container::UnorderedCollection
	<
		engine::Asset, 201,
		std::array<Simple, 100>,
		std::array<ColorSelector, 20>
	>;
	Resources resources;

	resource::ColorValue ColorSelector::get(const View * view) const
	{
		switch (view->status.state)
		{
		case Status::HIGHLIGHT:
			return resources.get<Simple>(this->state_highlight).color;
		case Status::PRESSED:
			return resources.get<Simple>(this->state_pressed).color;
		case Status::DEFAULT:
		default:
			return resources.get<Simple>(this->state_default).color;
		}
	}

	struct
	{
		const resource::Color * operator() (const Simple & val)
		{
			return &val;
		}

		const resource::Color * operator() (const ColorSelector & val)
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
		const Color * color(const engine::Asset asset)
		{
			return resources.call(asset, lookup_color);
		}

		void put(const engine::Asset asset, const ColorValue val)
		{
			resources.emplace<Simple>(asset, val);
		}

		void put(const engine::Asset asset, const engine::Asset def, const engine::Asset high, const engine::Asset press)
		{
			resources.emplace<ColorSelector>(asset, def, high, press);
		}

		void purge()
		{
			::resources = Resources{};
		}
	}
}
}
