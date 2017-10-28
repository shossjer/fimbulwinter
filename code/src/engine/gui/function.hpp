
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_FUNCTION_HPP
#define ENGINE_GUI_FUNCTION_HPP

#include <utility/variant.hpp>

namespace engine
{
	namespace gui
	{
		class View;

		class Function
		{
		public:

			struct List
			{
				// View * or entity
				// Item template
				// Active items?
			};

			struct Progress
			{
				// View * or entity
			};

			struct TabCantroller
			{
				// List<View * or entity> for tabs
				// List<View * or entity> for content
				// selection info
				// 
			};

			using Content = utility::variant
			<
				List,
				Progress,
				TabCantroller
			>;

		private:

			Content content;

			// The view which the functionality belongs to
			View *const view;

		public:

			Function(
				Content && content,
				View * view)
				: content(std::move(content))
				, view(view)
			{}
		};
	}
}

#endif
