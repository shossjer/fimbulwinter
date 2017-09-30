
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
			static void measure_active(View & view)
			{
				if (!view.change.affects_size())
					return;

				switch (view.size.height.type)
				{
				case Size::TYPE::FIXED:
					view.size.height.fixed();
					break;
				case Size::TYPE::PARENT:
				case Size::TYPE::PERCENT:
					// do nothing
					break;
				case Size::TYPE::WRAP:
					view.size.height.wrap(view.wrap_height());
					break;

				default:
					debug_unreachable();
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::FIXED:
					view.size.width.fixed();
					break;
				case Size::TYPE::PARENT:
				case Size::TYPE::PERCENT:
					// do nothing
					break;
				case Size::TYPE::WRAP:
					view.size.width.wrap(view.wrap_width());
					break;

				default:
					debug_unreachable();
				}

				// TODO: check if changed, clear "size" if unchanged!
			}

			static void measure_passive(View & view, const Group * const parent)
			{
				if (!view.change.affects_size())
					return;

				switch (view.size.height.type)
				{
				case Size::TYPE::FIXED:
				case Size::TYPE::PARENT:
				case Size::TYPE::WRAP:
					// do nothing
					break;
				case Size::TYPE::PERCENT:
					view.size.height.percentage(parent->get_size().height.value - view.margin.height());
					break;
				default:
					debug_unreachable();
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::FIXED:
				case Size::TYPE::PARENT:
				case Size::TYPE::WRAP:
					// do nothing
					break;
				case Size::TYPE::PERCENT:
					view.size.width.percentage(parent->get_size().width.value - view.margin.width());
					break;
				default:
					debug_unreachable();
				}

				// TODO: check if changed, clear "size" if unchanged!
			}

			static void measure_passive_forced(
				View & view,
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent)
			{
				// force update (parent has changed)

				switch (view.size.height.type)
				{
				case Size::TYPE::FIXED:
					// do nothing
					break;
				case Size::TYPE::PARENT:
					view.size.height.parent(size_parent.height.value - view.margin.height());
					break;
				case Size::TYPE::PERCENT:
					view.size.height.percentage(size_parent.height.value - view.margin.height());
					break;
				case Size::TYPE::WRAP:
					// do nothing
					break;
				default:
					debug_unreachable();
				}

				switch (view.size.width.type)
				{
				case Size::TYPE::FIXED:
					// do nothing
					break;
				case Size::TYPE::PARENT:
					view.size.width.parent(size_parent.width.value - view.margin.width());
					break;
				case Size::TYPE::PERCENT:
					view.size.width.percentage(size_parent.width.value - view.margin.width());
					break;
				case Size::TYPE::WRAP:
					// do nothing
					break;
				default:
					debug_unreachable();
				}

				//// TODO: have it here or in drawable?
				//const Vector3f ret = ViewMeasure::offset(view, size_parent, gravity_mask_parent, offset_parent);
				//view.offset = ret + Vector3f{ 0.f, 0.f, view.order_depth };
			}

			static Vector3f offset(
				const View & view,
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent)
			{
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
