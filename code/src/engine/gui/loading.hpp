
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

	struct ReactionData
	{
		union Node
		{
			engine::Asset key;
			int index;
		};
		std::vector<Node> observe;

		bool is_set() const
		{
			return !this->observe.empty();
		}
	};

	struct controller_data_t
	{
		ReactionData reaction;

		struct list_t
		{
			std::vector<DataVariant> item_template;
		};

		struct tab_t
		{
			std::string pager_name;
			std::string tabs_name;
		};

		using Variant = utility::variant
		<
			std::nullptr_t,
			list_t,
			tab_t
		>;

		Variant data;

		controller_data_t() : data(utility::in_place_type<std::nullptr_t>) {}
	};

	struct interaction_data_t
	{
		enum
		{
			CLOSE,
			INTERACTION,
			// TAB is only created programatically
			UNKNOWN
		}
		type;

		std::string target;

		auto has_target() const
		{
			return !target.empty();
		}
	};

	struct ViewData
	{
		std::string name;
		Size size;
		Margin margin;
		Gravity gravity;

		controller_data_t controller;
		std::vector<interaction_data_t> interactions;
		ReactionData reaction;

		ViewData(Size size)
			: name()
			, size(size)
			, margin()
			, gravity()
		{
		}

		bool has_controller() const
		{
			return !utility::holds_alternative<std::nullptr_t>(controller.data);
		}

		bool has_name() const
		{
			return !this->name.empty();
		}

		bool has_reaction() const
		{
			return !reaction.observe.empty();
		}

	private:
		virtual void hack() const {}
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
