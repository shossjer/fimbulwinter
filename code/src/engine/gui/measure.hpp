
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_MEASURE_HPP
#define ENGINE_GUI_MEASURE_HPP

#include "views.hpp"

#include <engine/debug.hpp>

namespace engine
{
	namespace gui
	{
		struct ViewMeasure
		{
			static void measure_active(Drawable & view)
			{
				// NOTE: could be made to handle H / V more separately
				if (!view.change.affects_size())
					return;

				switch (view.size.height.type)
				{
				case Size::TYPE::FIXED:
					if (!view.size.height.fixed())
					{
						view.change.clear_size_h();
					}
					break;
				case Size::TYPE::WRAP:
					if (!view.size.height.wrap(view.wrap_height()))
					{
						view.change.clear_size_h();
					}
					break;
				default:
					break;
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::FIXED:
					if (!view.size.width.fixed())
					{
						view.change.clear_size_w();
					}
					break;
				case Size::TYPE::WRAP:
					if (!view.size.width.wrap(view.wrap_width()))
					{
						view.change.clear_size_w();
					}
					break;
				default:
					break;
				}
			}

			static void measure_active(Group & view, const change_t children_changes)
			{
				// NOTE: could be made to handle H / V more separately

				switch (view.size.height.type)
				{
				case Size::TYPE::FIXED:
					if (view.change.affects_size())
					{
						if (view.size.height.fixed())
						{
							view.change.clear_size_h();
						}
					}
					break;
				case Size::TYPE::WRAP:
					if (children_changes.affects_size())
					{
						if (view.size.height.wrap(view.wrap_height()))
						{
							view.change.set_resized_h();
						}
						else
						{
							view.change.clear_size_h();
						}
					}
					break;

				default:
					break;
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::FIXED:
					if (view.change.affects_size())
					{
						if (view.size.width.fixed())
						{
							view.change.clear_size_w();
						}
					}
					break;
				case Size::TYPE::WRAP:
					if (children_changes.affects_size())
					{
						if (view.size.width.wrap(view.wrap_width()))
						{
							view.change.set_resized_w();
						}
						else
						{
							view.change.clear_size_w();
						}
					}
					break;

				default:
					break;
				}
			}

			// only updates if "percentage" (which is not "size" change)
			// updates state of "size" change
			static void measure_passive(View & view, const Group * const parent)
			{
				// NOTE: could be made to handle H / V more separately

				if (!view.change.affects_size())
					return;

				switch (view.size.height.type)
				{
				case Size::TYPE::PERCENT:
					if (!view.size.height.percentage(parent->get_size().height.value - view.margin.height()))
					{
						view.change.clear_size_h();
					}
					break;
				default:
					break;
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::PERCENT:
					if (!view.size.width.percentage(parent->get_size().width.value - view.margin.width()))
					{
						view.change.clear_size_w();
					}
					break;
				default:
					break;
				}
			}

			static void measure_passive_forced(View & view, const Size size_parent)
			{
				// NOTE: could be made to handle H / V more separately
				// force update (parent has changed)

				switch (view.size.height.type)
				{
				case Size::TYPE::PARENT:
					if (!view.size.height.parent(size_parent.height.value - view.margin.height()))
					{
						view.change.clear_size_h();
					}
					else
					{
						view.change.set_resized_h();
					}
					break;
				case Size::TYPE::PERCENT:
					if (!view.size.height.percentage(size_parent.height.value - view.margin.height()))
					{
						view.change.clear_size_h();
					}
					else
					{
						view.change.set_resized_h();
					}
					break;

				default:
					break;
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::PARENT:
					if (!view.size.width.parent(size_parent.width.value - view.margin.width()))
					{
						view.change.clear_size_w();
					}
					else
					{
						view.change.set_resized_w();
					}
					break;
				case Size::TYPE::PERCENT:
					if (!view.size.width.percentage(size_parent.width.value - view.margin.width()))
					{
						view.change.clear_size_w();
					}
					else
					{
						view.change.set_resized_w();
					}
					break;

				default:
					break;
				}
			}

			static Vector3f offset(
				const View & view,
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent)
			{
				// NOTE: could be made to handle H / V more separately
				Vector3f::array_type buff;
				offset_parent.get_aligned(buff);

				float offset_horizontal;

				if (view.gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_RIGHT))
				{
					offset_horizontal = buff[0] + size_parent.width.value - view.size.width.value - view.margin.right;
				}
				else if (view.gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_CENTRE))
				{
					offset_horizontal = buff[0] + (size_parent.width.value / 2) - (view.size.width.value / 2);
				}
				else // LEFT default
				{
					offset_horizontal = buff[0] + view.margin.left;
				}

				float offset_vertical;

				if (view.gravity.place(gravity_mask_parent, Gravity::VERTICAL_BOTTOM))
				{
					offset_vertical = buff[1] + size_parent.height.value - view.size.height.value - view.margin.bottom;
				}
				else if (view.gravity.place(gravity_mask_parent, Gravity::VERTICAL_CENTRE))
				{
					offset_vertical = buff[1] + (size_parent.height.value / 2) - (view.size.height.value / 2) + view.margin.top - view.margin.bottom;
				}
				else // TOP default
				{
					offset_vertical = buff[1] + view.margin.top;
				}

				return Vector3f(offset_horizontal, offset_vertical, buff[2]);
			}
		};
	}
}

#endif // ENGINE_GUI_MEASURE_HPP
