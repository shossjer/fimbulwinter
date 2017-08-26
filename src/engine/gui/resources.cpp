
#include "resources.hpp"

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
		Color get(const View * view) const override
		{
			return 0;
		}
	};

	core::container::UnorderedCollection
	<
		engine::Asset, 201,
		std::array<Simple, 100>,
		std::array<ColorSelector, 20>
	>
	resources;

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
	}
}
}
