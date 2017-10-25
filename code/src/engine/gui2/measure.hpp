
// should not be included outside gui namespace.

#ifndef ENGINE_GUI2_MEASURE_HPP
#define ENGINE_GUI2_MEASURE_HPP

#include "render.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui2
	{
		// progress on any "path" which is "changed"
		// this will be basically "refresh2" from before

		// Changes
		//	CHILD,
		//	CONTENT,
		//	VISIBILITY,
		//	SIZE_H,
		//	SIZE_V

		struct ViewMeasure
		{
			static void refresh(View & view);
			static void refresh(View & view, const Offset & offset, const Size & size);

		private:

			struct ContentRefresh
			{
				View & view;

				template<typename T>
				void operator() (T & content)
				{
					renderer_refresh(content);
				}
				void operator() (View::Group & content)
				{
					// allows for children to update
					for (auto child : content.children)
					{
						refresh(*child);
					}
				}
			};
			struct ContentResize
			{
				View & view;

				template<typename T>
				void operator() (T & content)
				{
					renderer_refresh(content);
				}
				void operator() (View::Group & content)
				{
					// Purpose - update children after 'child size-change' / 'group changed'
					// Offset needs to be updated for Horizontal / Vertical
					// Size needs to be updated for any child with Size::PARENT

					// Assumption: View size is already updated (either by parent or during setup or during child update).
				
					Gravity mask;
					Offset offset = this->view.offset;
					Size size = this->view.size;

					switch (content.layout)
					{
					case View::Group::Layout::HORIZONTAL:

						mask = Gravity{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

						for (auto child : content.children)
						{
							refresh(*child, offset, size);

							const auto child_size = child->width();
							offset += child_size;
							size -= child_size;
						}

						break;

					case View::Group::Layout::VERTICAL:

						mask = Gravity{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

						for (auto child : content.children)
						{
							refresh(*child, offset, size);

							const auto child_size = child->height();
							offset += child_size;
							size -= child_size;
						}

						break;

					case View::Group::Layout::RELATIVE:
					default:

						mask = Gravity::unmasked();

						for (auto child : content.children)
						{
							refresh(*child, offset, size);
						}

						break;
					}

					this->view.change.clear();
				}
			};
		};

		// View to be updated if changed.
		void ViewMeasure::refresh(View & view)
		{
			if (!view.change.any())
				return;

			if (view.change.affects_size())
			{
				visit(ContentResize{ view }, view.content);
			}
			else if (view.change.affects_offset())
			{
				visit(ContentResize{ view }, view.content);
			}
			else
			{
				visit(ContentRefresh{ view }, view.content);
			}
		}

		// View to be updated based on parents changes.
		// Offset for the view (need to check if changed).
		// Size "remaining" for the view, based on parent group's layout.
		void ViewMeasure::refresh(View & view, const Offset & offset, const Size & size)
		{
			// FIXED, WRAP
			//   Change: can be any
			//   Size: already updated
			//   Offset: can be changed (based on parent changes and view gravity)

			// PARENT:
			//   Changes: can be any
			//   Size: can/will be changed here
			//   Offset: can be changed (based on parent changes and view gravity)

			if (view.offset != offset)	// Note: could check dimen's separately
			{
				view.offset = offset;
				view.change.set_moved();
			}

			switch (view.size.height.type)
			{
			case Size::PARENT:
				// check and set if changed
				if (view.size.update_parent(view.margin, size.height))
				{
					view.size += size.height;
					view.change.set_resized_h();
				}
				break;

			case Size::FIXED:
			case Size::WRAP:
			default:
				// nothing to do
				// Note: could verify view still fits inside parent
				break;
			}

			switch (view.size.width.type)
			{
			case Size::PARENT:
				// check and set if changed
				if (view.size.update_parent(view.margin, size.width))
				{
					view.size += size.width;
					view.change.set_resized_w();
				}
				break;

			case Size::FIXED:
			case Size::WRAP:
			default:
				// nothing to do
				// Note: could verify view still fits inside parent
				break;
			}

			// checks for any changes and updates if so.
			refresh(view);
		}
	}
}

#endif
