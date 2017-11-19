
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_VIEW_HPP
#define ENGINE_GUI_VIEW_HPP

#include "common.hpp"
#include "resources.hpp"

#include <engine/debug.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui
	{
		class Function;

		class View
		{
			friend struct ViewAccess;
			friend struct ViewMeasure;
			friend struct ViewUpdater;

		public:

			struct Group
			{
				enum Layout
				{
					HORIZONTAL,
					VERTICAL,
					RELATIVE
				};

				Layout layout;
				std::vector<View *> children;

				void adopt(View * child) { this->children.push_back(child); }

				void abandon(View * child)
				{
					auto itr = std::find(this->children.begin(), this->children.end(), child);
					if (itr == this->children.end())
					{
						debug_printline(engine::gui_channel, "Child missing from parent.");
						debug_unreachable();
					}
					this->children.erase(itr);
				}
			};

			struct Color
			{
				resource::Color const * color;
			};

			struct Text
			{
				std::string display;
				resource::Color const * color;
			};

			struct Texture
			{
				engine::Asset res;
			};

			using Content = utility::variant
			<
				Group,
				Color,
				Text,
				Texture
			>;

		public:

			Entity entity;

			Content content;

			Gravity gravity;

			Margin margin;

			Offset offset;

			Size size;

			View * parent;

			Change change;

			Status status;

			// remove this somehow
			float depth;

			// remove this somehow
			bool selectable;

			// would be nice if it could be removed...
			Function * function;

		public:

			View(
				Entity entity,
				Content && content,
				Gravity gravity,
				Margin margin,
				Size size,
				View *const parent)
				: entity(entity)
				, content(std::move(content))
				, gravity(gravity)
				, margin(margin)
				, size(size)
				, parent(parent)
				, change()
				, status()
				, selectable(false)
				, function(nullptr)
			{}
		};
	}
}

#endif
