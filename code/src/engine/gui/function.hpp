
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_FUNCTION_HPP
#define ENGINE_GUI_FUNCTION_HPP

#include "loading.hpp"

#include <engine/Entity.hpp>

#include <utility/variant.hpp>

#include <vector>

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
				DataVariant item_template;
				std::vector<View *> items;
			};

			struct Progress
			{
				// View * or entity
			};

			struct TabCantroller
			{
				std::vector<View *> tab_groups;
				std::vector<View *> page_groups;
				std::size_t active_index = 0;
			};

			using Content = utility::variant
			<
				List,
				Progress,
				TabCantroller
			>;

		public:

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

		namespace fun
		{
			struct Trigger
			{
				Entity caller;
				Function * function;

				void operator() (Function::TabCantroller & content);

				template<typename T>
				void operator() (const T & content)
				{
					debug_unreachable();
				}
			};
		}
	}
}

#endif
