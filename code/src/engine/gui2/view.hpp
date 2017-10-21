
// should not be included outside gui namespace.

#ifndef ENGINE_GUI2_VIEW_HPP
#define ENGINE_GUI2_VIEW_HPP

#include "common.hpp"
#include "resources.hpp"

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui2
	{
		class View
		{
			friend struct ViewMeasure;
			friend struct ViewRefresh;
			friend struct ViewUpdater;
			friend struct ViewTester;

		public:

			struct Group
			{
				enum class Layout
				{
					HORIZONTAL,
					VERTICAL,
					RELATIVE
				};

				std::vector<View *> children;
				Layout layout;
			};

			struct Color
			{
				resource::Color value;
			};

			struct Text
			{
				std::string display;
				resource::Color value;
			};

			using Content = utility::variant
			<
				Group,
				Color,
				Text
			>;

		private:

			Content content;

			Gravity gravity;

			Margin margin;

			Offset offset;

			Size size;

			View *const parent;

		private:

			Change change;

			Status status;

		public:

			View(
				Content && content,
				Gravity gravity,
				Margin margin,
				Size size,
				View *const parent)
				: content(std::move(content))
				, gravity(gravity)
				, margin(margin)
				, size(size)
				, parent(parent)
				, change()
				, status()
			{}

		public:

			auto height() const { return this->margin.height() + this->size.height; }
			auto width() const { return this->margin.width() + this->size.width; }
		};
	}
}

#endif
