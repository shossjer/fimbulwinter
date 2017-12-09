
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_MEASURE_HPP
#define ENGINE_GUI_MEASURE_HPP

#include "render.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
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
			// View to be updated if changed.
			static void refresh(View & view)
			{
				if (!view.change.any())
					return;

				if (view.change.affects_size())
				{
					visit(ContentResize{ view }, view.content);
				}
				else if (view.change.affects_moved())
				{
					visit(ContentResize{ view }, view.content);
				}
				else
				{
					visit(ContentRefresh{ view }, view.content);
				}

				view.change.clear();
			}

			// View to be updated based on parents changes.
			// Offset for the view (need to check if changed).
			// Size "remaining" for the view, based on parent group's layout.
			static void refresh(View & view, const Gravity & mask, const Offset & offset, const Size & size)
			{
				// FIXED, WRAP
				//   Change: can be any
				//   Size: already updated
				//   Offset: can be changed (based on parent changes and view gravity)

				// PARENT:
				//   Changes: can be any
				//   Size: can/will be changed here
				//   Offset: can be changed (based on parent changes and view gravity)

				const Gravity gravity = view.gravity & mask;

				switch (view.size.height.type)
				{
				case Size::PARENT:
					// check and set if changed
					if (view.size.update_parent(view.margin, size.height))
					{
						view.change.set_resized_h();
					}
					if (view.offset.update(offset.height + view.margin.top))
					{
						view.change.set_moved_h();
					}
					break;

				case Size::FIXED:
				case Size::WRAP:
				default:
					// Note: could verify view still fits inside parent

					if (gravity == Gravity::VERTICAL_BOTTOM)
					{
						if (view.offset.update(offset.height + (size.height - view.size.height - view.margin.bottom)))
						{
							view.change.set_moved_h();
						}
					}
					else if (gravity == Gravity::VERTICAL_CENTRE)
					{
						if (view.offset.update(offset.height + ((size.height - view.size.height) / 2) + (view.margin.top - view.margin.bottom)))
						{
							view.change.set_moved_h();
						}
					}
					else
					{
						if (view.offset.update(offset.height + view.margin.top))
						{
							view.change.set_moved_h();
						}
					}
					break;
				}

				switch (view.size.width.type)
				{
				case Size::PARENT:
					// check and set if changed
					if (view.size.update_parent(view.margin, size.width))
					{
						view.change.set_resized_w();
					}
					if (view.offset.update(offset.width + view.margin.left))
					{
						view.change.set_moved_w();
					}
					break;

				case Size::FIXED:
				case Size::WRAP:
				default:
					// Note: could verify view still fits inside parent

					if (gravity == Gravity::HORIZONTAL_RIGHT)
					{
						if (view.offset.update(offset.width + (size.width - view.size.width - view.margin.right)))
						{
							view.change.set_moved_w();
						}
					}
					else if (gravity == Gravity::HORIZONTAL_CENTRE)
					{
						if (view.offset.update(offset.width + ((size.width - view.size.width) / 2) + (view.margin.left - view.margin.right)))
						{
							view.change.set_moved_w();
						}
					}
					else
					{
						if (view.offset.update(offset.width + view.margin.left))
						{
							view.change.set_moved_w();
						}
					}
					break;
				}
			}

		private:

			static auto height(View const * view)
			{
				return view->margin.height() + view->size.height;
			}
			static auto width(View const * view)
			{
				return view->margin.width() + view->size.width;
			}

			struct ContentRefresh
			{
				View & view;

				template<typename T>
				void operator() (T & content)
				{
					ViewRenderer::refresh(view, content);
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
					ViewRenderer::refresh(view, content);
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
							refresh(*child, mask, offset, size);
							refresh(*child);

							const auto child_size = width(child);
							offset += child_size;
							size -= child_size;
						}

						break;

					case View::Group::Layout::VERTICAL:

						mask = Gravity{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

						for (auto child : content.children)
						{
							refresh(*child, mask, offset, size);
							refresh(*child);

							const auto child_size = height(child);
							offset += child_size;
							size -= child_size;
						}

						break;

					case View::Group::Layout::RELATIVE:
					default:

						mask = Gravity::unmasked();

						for (auto child : content.children)
						{
							refresh(*child, mask, offset, size);
							refresh(*child);
						}

						break;
					}

					this->view.change.clear();
				}
			};
		};
	}
}

#endif
