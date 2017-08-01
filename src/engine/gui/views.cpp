
#include "views.hpp"

#include <core/container/Collection.hpp>

#include <utility/json.hpp>

#include <fstream>

// TODO: this should be solved better.
namespace gameplay
{
namespace gamestate
{
	void post_add(engine::Entity entity, engine::Asset window, engine::Asset name);
}
}

namespace
{
	typedef float value_t;
	typedef engine::graphics::data::Color Color;

	constexpr float DEPTH_INC = .1f;

	class View
	{
	public:

		struct Gravity
		{
			static constexpr uint_fast16_t HORIZONTAL_LEFT = 1 << 0;
			static constexpr uint_fast16_t HORIZONTAL_CENTRE = 1 << 1;
			static constexpr uint_fast16_t HORIZONTAL_RIGHT = 1 << 2;

			static constexpr uint_fast16_t VERTICAL_TOP = 1 << 3;
			static constexpr uint_fast16_t VERTICAL_CENTRE = 1 << 4;
			static constexpr uint_fast16_t VERTICAL_BOTTOM = 1 << 5;

		private:

			uint_fast16_t flags;

		public:

			Gravity()
				: flags(HORIZONTAL_LEFT | VERTICAL_TOP)
			{}

			Gravity(const uint_fast16_t flags)
				: flags(flags)
			{}

			static Gravity unmasked()
			{
				return Gravity{ 0xFFFF };
			}

		public:

			// checks is Gravity is set and allowed by parent
			bool place(const Gravity mask, const uint_fast16_t flag) const
			{
				return ((this->flags & mask.flags) & flag) != 0;
			}
		};

		struct Margin
		{
			value_t left;
			value_t right;
			value_t top;
			value_t bottom;

			Margin(
				value_t left = value_t{ 0 },
				value_t right = value_t{ 0 },
				value_t top = value_t{ 0 },
				value_t bottom = value_t{ 0 })
				: left(left)
				, right(right)
				, top(top)
				, bottom(bottom)
			{}

			value_t width() const { return this->left + this->right; }

			value_t height() const { return this->top + this->bottom; }
		};

		struct Size
		{
			enum class TYPE
			{
				FIXED,
				PARENT,
				PERCENTAGE,
				// WEIGHT
				WRAP
			};

			struct Dimen
			{
				const TYPE type;

				// the fixed / percentage / weight value
				value_t meta;

				// the calculated used size value
				value_t value;

				Dimen(TYPE type)
					: type(type)
					, meta(value_t{ 0 })
					, value(value_t{ 0 })
				{
					debug_assert(type != TYPE::FIXED);
					debug_assert(type != TYPE::PERCENTAGE);
				}

				Dimen(TYPE type, value_t meta)
					: type(type)
					, meta(meta)
					, value(value_t{ 0 })
				{
					debug_assert(type != TYPE::PARENT);
					debug_assert(type != TYPE::WRAP);
				}

				void fixed(const value_t max_size, const value_t margin)
				{
					debug_assert(max_size >= (this->meta + margin));
					this->value = this->meta;
				}

				void parent(const value_t max_size)
				{
					this->value = max_size;
				}

				void percentage(const value_t max_size)
				{
					this->value = max_size * this->meta;
				}

				// limit value, used with wrap content
				void limit(const value_t value)
				{
					debug_assert(this->value <= value);
					this->value = value;
				}

				void set_meta(const value_t meta)
				{
					this->meta = meta;
				}

				bool operator == (const TYPE type) const
				{
					return this->type == type;
				}

				void operator-=(const value_t amount)
				{
					this->value -= amount;
				}
				void operator-=(const Dimen other)
				{
					this->value -= other.value;
				}
			};

			Dimen width;
			Dimen height;
		};

	public:

		const Gravity gravity;

	protected:

		const Margin margin;

		Size size;

	protected:

		View(
			Gravity gravity,
			Margin margin,
			Size size)
			: gravity(gravity)
			, margin(margin)
			, size(size)
		{}

	public:

		// called after size has been measured
		virtual void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) = 0;

		// needs to be called after any changes has been made to Components
		virtual void measure(
			const Size size_parent) = 0;

		virtual void show(const Vector3f delta) = 0;

		virtual void hide() = 0;

		virtual void translate(const Vector3f delta) = 0;

		// total height of the view including margins
		value_t height() const
		{
			return this->margin.height() + this->size.height.value;
		}

		// total width of the view including margins
		value_t width() const
		{
			return this->margin.width() + this->size.width.value;
		}

	protected:

		// call after size has been assigned
		Vector3f arrange_offset(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) const
		{
			Vector3f::array_type buff;
			offset_parent.get_aligned(buff);

			float offset_horizontal;

			if (this->gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_RIGHT))
			{
				offset_horizontal = buff[0] + size_parent.width.value - this->size.width.value - this->margin.right;
			}
			else
			if (this->gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_CENTRE))
			{
				offset_horizontal = buff[0] + (size_parent.width.value / 2) - (this->size.width.value / 2);
			}
			else // LEFT default
			{
				offset_horizontal = buff[0] + this->margin.left;
			}

			float offset_vertical;

			if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_BOTTOM))
			{
				offset_vertical = buff[1] + size_parent.height.value - this->size.height.value - this->margin.bottom;
			}
			else
			if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_CENTRE))
			{
				offset_vertical = buff[1] + (size_parent.height.value / 2) - (this->size.height.value / 2) + this->margin.top - this->margin.bottom;
			}
			else // TOP default
			{
				offset_vertical = buff[1] + this->margin.top;
			}

			return Vector3f(offset_horizontal, offset_vertical, buff[2]);
		}
	};

	class Drawable : public View
	{
		// turned into matrix and sent to renderer
		Vector3f offset;

	public:

		engine::Entity entity;

		bool selectable;

	protected:

		Drawable(engine::Entity entity, Gravity gravity, Margin margin, Size size, bool selectable)
			: View(gravity, margin, size)
			, entity(entity)
			, selectable(selectable)
		{
		}

	protected:

		void update_translation(core::maths::Vector3f delta)
		{
			this->offset += delta;
		}

	public:

		auto render_matrix() const
		{
			return make_translation_matrix(this->offset);
		}

		auto render_size() const
		{
			return core::maths::Vector2f{
				static_cast<float>(this->size.width.value),
				static_cast<float>(this->size.height.value) };
		}

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override
		{
			const Vector3f ret = arrange_offset(size_parent, gravity_mask_parent, offset_parent);
			this->offset = ret + Vector3f{ 0.f, 0.f, offset_depth };
			offset_depth += DEPTH_INC;
		}

		void hide() override
		{
			// TODO: check if view is visible

			engine::graphics::renderer::post_remove(this->entity);
		}

		void translate(core::maths::Vector3f delta) override
		{
			// TODO: check if view is visible

			update_translation(delta);

			// send the updated matrix
			engine::graphics::renderer::post_update_modelviewmatrix(
				this->entity,
				engine::graphics::data::ModelviewMatrix{ render_matrix() });
		}
	};

	class PanelC : public Drawable
	{
	public:

		Color color;

	public:

		PanelC(engine::Entity entity, Gravity gravity, Margin margin, Size size, Color color, bool selectable)
			: Drawable(entity, gravity, margin, size, selectable)
			, color(color)
		{
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

		void measure(const Size parent) override
		{
			{
				const auto max_height = parent.height.value - this->margin.height();

				switch (this->size.height.type)
				{
				case Size::TYPE::FIXED:
					this->size.height.fixed(max_height, this->margin.height());
					break;
				case Size::TYPE::PARENT:
					this->size.height.parent(max_height);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.height.percentage(max_height);
					break;

				default:
					debug_unreachable();
				}
			}
			{
				const auto max_width = parent.width.value - this->margin.width();

				switch (this->size.width.type)
				{
				case Size::TYPE::FIXED:
					this->size.width.fixed(max_width, this->margin.width());
					break;
				case Size::TYPE::PARENT:
					this->size.width.parent(max_width);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.width.percentage(max_width);
					break;

				default:
					debug_unreachable();
				}
			}
		}

		void show(const Vector3f delta) override
		{
			// TODO: check that view is not hidden

			update_translation(delta);

			engine::graphics::renderer::post_add_panel(
				this->entity,
				engine::graphics::data::ui::PanelC{
					this->render_matrix(),
					this->render_size(),
					this->color });

			if (this->selectable)
			{
				engine::graphics::renderer::post_make_selectable(this->entity);
			}
		}
	};

	class PanelT : public Drawable
	{
	public:

		engine::Asset texture;

	public:

		PanelT(engine::Entity entity, Gravity gravity, Margin margin, Size size, engine::Asset texture, bool selectable)
			: Drawable(entity, gravity, margin, size, selectable)
			, texture(texture)
		{
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

		void measure(const Size parent) override
		{
			{
				const auto max_height = parent.height.value - this->margin.height();

				switch (this->size.height.type)
				{
				case Size::TYPE::FIXED:
					this->size.height.fixed(max_height, this->margin.height());
					break;
				case Size::TYPE::PARENT:
					this->size.height.parent(max_height);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.height.percentage(max_height);
					break;

				default:
					debug_unreachable();
				}
			}
			{
				const auto max_width = parent.width.value - this->margin.width();

				switch (this->size.width.type)
				{
				case Size::TYPE::FIXED:
					this->size.width.fixed(max_width, this->margin.width());
					break;
				case Size::TYPE::PARENT:
					this->size.width.parent(max_width);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.width.percentage(max_width);
					break;

				default:
					debug_unreachable();
				}
			}
		}

		void show(const Vector3f delta) override
		{
			// TODO: check that view is not hidden

			update_translation(delta);

			engine::graphics::renderer::post_add_panel(
				this->entity,
				engine::graphics::data::ui::PanelT{
				this->render_matrix(),
					this->render_size(),
					this->texture });

			if (this->selectable)
			{
				engine::graphics::renderer::post_make_selectable(this->entity);
			}
		}
	};

	class Text : public Drawable
	{
	public:

		Color color;
		std::string display;

	public:

		Text(engine::Entity entity, Gravity gravity, Margin margin, Size size, Color color, std::string display)
			: Drawable(entity, gravity, margin, size, false)
			, color(color)
			, display(display)
		{
		}

		void measure(const Size parent) override
		{
			{
				const auto max_height = parent.height.value - this->margin.height();

				switch (this->size.height.type)
				{
				case Size::TYPE::FIXED:
					this->size.height.fixed(max_height, this->margin.height());
					break;
				case Size::TYPE::PARENT:
					this->size.height.parent(max_height);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.height.percentage(max_height);
					break;
				case Size::TYPE::WRAP:
					this->size.height.value = 6;
					break;
				default:
					debug_unreachable();
				}
			}
			{
				const auto max_width = parent.width.value - this->margin.width();

				switch (this->size.width.type)
				{
				case Size::TYPE::FIXED:
					this->size.width.fixed(max_width, this->margin.width());
					break;
				case Size::TYPE::PARENT:
					this->size.width.parent(max_width);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.width.percentage(max_width);
					break;
				case Size::TYPE::WRAP:
					this->size.width.value = this->display.length() * value_t { 6 };
					break;

				default:
					debug_unreachable();
				}
			}
		}

		void show(const Vector3f delta) override
		{
			// TODO: check that view is not hidden

			update_translation(delta);

			engine::graphics::renderer::post_add_text(
				this->entity,
				engine::graphics::data::ui::Text{
				this->render_matrix(),
					this->color,
					this->display });

			if (this->selectable)
			{
				engine::graphics::renderer::post_make_selectable(this->entity);
			}
		}
	};

	class Group : public View
	{
	public:

		enum class Layout
		{
			HORIZONTAL,
			VERTICAL,
			RELATIVE
		};

	private:

		std::vector<View*> children;

		Layout layout;

	public:

		Group(Gravity gravity, Margin margin, Size size, Layout layout)
			: View(gravity, margin, size)
			, layout(layout)
		{}

	protected:

		void measure_children()
		{
			Size size_children = this->size;

			switch (this->layout)
			{
			case Layout::HORIZONTAL:
				{
					for (auto p : this->children)
					{
						p->measure(size_children);
						size_children.width -= p->width();
					}

					if (this->size.width == Size::TYPE::WRAP)
						this->size.width -= size_children.width;

					break;
				}
			case Layout::VERTICAL:
				{
					for (auto p : this->children)
					{
						p->measure(size_children);
						size_children.height -= p->height();
					}

					if (this->size.height == Size::TYPE::WRAP)
						this->size.height -= size_children.height;

					break;
				}
			case Layout::RELATIVE:
			default:
				{
					for (auto p : this->children)
					{
						p->measure(this->size);

						// used if this size is wrap
						size_children.width.value = std::max(size_children.width.value, p->width());
						size_children.height.value = std::max(size_children.height.value, p->width());
					}

					if (this->size.width == Size::TYPE::WRAP)
						this->size.width.value = size_children.width.value;

					if (this->size.height == Size::TYPE::WRAP)
						this->size.height.value = size_children.height.value;

					break;
				}
			}
		}

		void arrange_children(Vector3f offset, float & offset_depth)
		{
			switch (this->layout)
			{
			case Layout::HORIZONTAL:
				{
					const Gravity mask{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

					for (auto p : this->children)
					{
						p->arrange(this->size, mask, offset, offset_depth);
						offset += Vector3f(static_cast<float>(p->width()), 0.f, 0.f);
					}

					break;
				}
			case Layout::VERTICAL:
				{
					const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

					for (auto p : this->children)
					{
						p->arrange(this->size, mask, offset, offset_depth);
						offset += Vector3f(0.f, static_cast<float>(p->height()), 0.f);
					}

					break;
				}
			case Layout::RELATIVE:
			default:
				{
					const Gravity mask = Gravity::unmasked();

					for (auto p : this->children)
					{
						p->arrange(this->size, mask, offset, offset_depth);
					}

					break;
				}
			}
		}

	public:

		void adopt(View * child)
		{
			this->children.push_back(child);
		}

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override
		{
			Vector3f offset = arrange_offset(size_parent, gravity_mask_parent, offset_parent);

			arrange_children(offset, offset_depth);
		}

		void measure(const Size parent) override
		{
			{
				const auto max_height = parent.height.value - this->margin.height();

				switch (this->size.height.type)
				{
				case Size::TYPE::FIXED:
					this->size.height.fixed(max_height, this->margin.height());
					break;
				case Size::TYPE::PARENT:
					this->size.height.parent(max_height);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.height.percentage(max_height);
					break;
				case Size::TYPE::WRAP:
					this->size.height.value = parent.height.value;
					break;

				default:
					debug_unreachable();
				}
			}
			{
				const auto max_width = parent.width.value - this->margin.width();

				switch (this->size.width.type)
				{
				case Size::TYPE::FIXED:
					this->size.width.fixed(max_width, this->margin.width());
					break;
				case Size::TYPE::PARENT:
					this->size.width.parent(max_width);
					break;
				case Size::TYPE::PERCENTAGE:
					this->size.width.percentage(max_width);
					break;
				case Size::TYPE::WRAP:
					this->size.width.value = parent.width.value;
					break;

				default:
					debug_unreachable();
				}
			}

			measure_children();
		}

		void show(const Vector3f delta) override
		{
			for (auto child : this->children)
			{
				child->show(delta);
			}
		}

		void hide() override
		{
			for (auto child : this->children)
			{
				child->hide();
			}
		}

		void translate(const Vector3f delta) override
		{
			for (auto child : this->children)
			{
				child->translate(delta);
			}
		}
	};

	class Window : private Group
	{
		friend struct Components;

	public:

		engine::Asset name;

	private:

		Vector3f position;

		bool shown;

		// used to determine the drawing order / depth of the window components.
		int order;

	public:

		Window(engine::Asset name, Size size, Layout layout, Margin margin)
			: Group{ Gravity{}, Margin{}, size, layout }
			, name(name)
			, position({ static_cast<float>(margin.left), static_cast<float>(margin.top), 0.f })
			, shown(false)
			, order(0)
		{
			debug_assert(size.height.type != Size::TYPE::PARENT);
			debug_assert(size.width.type != Size::TYPE::PARENT);
		}

		bool isShown() const { return this->shown; }

		void show_window()
		{
			if (this->shown) return;

			this->shown = true;

			const Vector3f delta{ 0.f, 0.f, static_cast<float>(-(this->order)) };

			// move windows order to first when shown
			this->order = 0;
			this->position += delta;

			Group::show(delta);
		}

		void hide_window()
		{
			debug_assert(this->shown);
			this->shown = false;

			Group::hide();
		}

		void measure_window()
		{
			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				this->size.height.value = this->size.height.meta;
				break;

			default:
				debug_unreachable();
			}
			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				this->size.width.value = this->size.width.meta;
				break;

			default:
				debug_unreachable();
			}

			measure_children();

			float depth = static_cast<float>(this->order);
			arrange_children(this->position, depth);
		}

		void translate_window(const Vector3f delta)
		{
			this->position += delta;

			Group::translate(delta);
		}

		void reorder_window(const int window_order)
		{
			if (!this->shown) return;
			if (this->order == window_order) return;

			const float delta_depth = static_cast<float>(window_order - this->order);

			this->order = window_order;

			translate_window(Vector3f{ 0.f, 0.f, delta_depth });
		}
	};

	struct ResourceLoader
	{
	private:
		const json & jcolors;
		const json & jdimensions;
		const json & jstrings;

	public:

		ResourceLoader(const json & jcontent)
			: jcolors(jcontent["colors"])
			, jdimensions(jcontent["dimensions"])
			, jstrings(jcontent["strings"])
		{
			// load colors and themes and strings? into resources...
		}

	private:

		engine::graphics::data::Color extract_color(const json & jdata)
		{
			const std::string str = jdata["color"];
			debug_assert(str.length() > std::size_t{ 0 });

			if (str[0] == '#')
			{
				std::string res = this->jcolors[str];
				debug_assert(res.length() > std::size_t{ 0 });

				return std::stoul(res, nullptr, 16);
			}

			return std::stoul(str, nullptr, 16);
		}

		value_t extract_margin(const std::string key, const json & jmargin)
		{
			if (!contains(jmargin, key))
				return 0;

			const json & jv = jmargin[key];

			if (!jv.is_string())
				return jv.get<value_t>();

			const std::string str = jv;
			debug_assert(str.length() > std::size_t{ 0 });
			debug_assert(str[0] == '#');
			return this->jdimensions[str].get<value_t>();
		}

		View::Size::Dimen extract_dimen(const json & jd)
		{
			if (jd.is_string())
			{
				const std::string str = jd;

				debug_assert(str.length() > std::size_t{ 0 });

				if (str[0] == '#')
					return View::Size::Dimen{
						View::Size::TYPE::FIXED,
						this->jdimensions[str].get<value_t>() };

				if (str[str.length() - 1] == '%')
				{
					value_t val = std::stof(str.substr(0, str.length() - 1));
					return View::Size::Dimen{ View::Size::TYPE::PERCENTAGE, val / value_t{ 100 } };
				}
				if (str == "parent") return View::Size::Dimen{ View::Size::TYPE::PARENT };
				if (str == "wrap") return View::Size::Dimen{ View::Size::TYPE::WRAP };

				debug_printline(0xffffffff, "GUI - invalid size dimen: ", str);
				debug_unreachable();
			}

			return View::Size::Dimen{ View::Size::TYPE::FIXED, jd.get<value_t>() };
		}

		View::Size extract_size(const json & jdata)
		{
			const json jsize = jdata["size"];

			debug_assert(contains(jsize, "w"));
			debug_assert(contains(jsize, "h"));

			return View::Size{ extract_dimen(jsize["w"]), extract_dimen(jsize["h"]) };
		}

	public:

		engine::graphics::data::Color color(const json & jdata)
		{
			debug_assert(contains(jdata, "color"));
			return extract_color(jdata);
		}

		engine::graphics::data::Color color(const json & jdata, const engine::graphics::data::Color def_val)
		{
			if (!contains(jdata, "color")) return def_val;
			return extract_color(jdata);
		}

		Group::Layout layout(const json & jgroup)
		{
			debug_assert(contains(jgroup, "layout"));

			const std::string str = jgroup["layout"];

			if (str == "horizontal")return Group::Layout::HORIZONTAL;
			if (str == "vertical") return Group::Layout::VERTICAL;
			if (str == "relative") return Group::Layout::RELATIVE;

			debug_printline(0xffffffff, "GUI - invalid layout: ", str);
			debug_unreachable();
		}

		View::Gravity gravity(const json & jdata)
		{
			uint_fast16_t h = View::Gravity::HORIZONTAL_LEFT;
			uint_fast16_t v = View::Gravity::VERTICAL_TOP;

			if (contains(jdata, "gravity"))
			{
				const json jgravity = jdata["gravity"];

				if (contains(jgravity, "h"))
				{
					const std::string str = jgravity["h"];

					if (str == "left") h = View::Gravity::HORIZONTAL_LEFT;
					else
					if (str == "centre") h = View::Gravity::HORIZONTAL_CENTRE;
					else
					if (str == "right") h = View::Gravity::HORIZONTAL_RIGHT;
					else
					{
						debug_printline(0xffffffff, "GUI - invalid horizontal gravity: ", str);
						debug_unreachable();
					}
				}

				if (contains(jgravity, "v"))
				{
					const std::string str = jgravity["v"];

					if (str == "top") v = View::Gravity::VERTICAL_TOP;
					else
					if (str == "centre") v = View::Gravity::VERTICAL_CENTRE;
					else
					if (str == "bottom") v = View::Gravity::VERTICAL_BOTTOM;
					else
					{
						debug_printline(0xffffffff, "GUI - invalid vertical gravity: ", str);
						debug_unreachable();
					}
				}
			}

			return View::Gravity{ h | v };
		}

		View::Margin margin(const json & jdata)
		{
			View::Margin margin;

			if (contains(jdata, "margin"))
			{
				const json jmargin = jdata["margin"];

				margin.bottom = extract_margin("b", jmargin);
				margin.left = extract_margin("l", jmargin);
				margin.right = extract_margin("r", jmargin);
				margin.top = extract_margin("t", jmargin);
			}

			return margin;
		}

		View::Size size(const json & jdata)
		{
			debug_assert(contains(jdata, "size"));
			return extract_size(jdata);
		}

		View::Size size_def_parent(const json & jdata)
		{
			if (!contains(jdata, "size"))
				return View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT };
			return extract_size(jdata);
		}

		View::Size size_def_wrap(const json & jdata)
		{
			if (!contains(jdata, "size"))
				return View::Size{ View::Size::TYPE::WRAP, View::Size::TYPE::WRAP };
			return extract_size(jdata);
		}
	};

	struct CloseAction
	{
		engine::Asset window;
	};

	core::container::Collection
		<
		engine::Entity, 201,
		std::array<CloseAction, 10>,
		std::array<Group, 40>,
		std::array<PanelC, 50>,
		std::array<PanelT, 50>,
		std::array<Text, 50>
		>
		components;

	// lookup table for "named" components.
	core::container::Collection
		<
		engine::Asset, 201,
		std::array<engine::Entity, 100>,
		std::array<int, 1>
		>
		lookup;

	struct Components
	{
		Window & window;
		ResourceLoader & load;

		Components(Window & window, ResourceLoader & load)
			: window(window)
			, load(load)
		{}

	private:

		bool has_action(const json & jcomponent) { return contains(jcomponent, "trigger"); }

		void load_action(engine::Entity entity, const json & jcomponent)
		{
			const json & jtrigger = jcomponent["trigger"];

			engine::Asset action { jtrigger["action"].get<std::string>() };

			switch (action)
			{
			case engine::Asset{ "close" }:

				components.emplace<CloseAction>(entity, CloseAction{ this->window.name });
				break;

			case engine::Asset{ "mover" }:

				// TODO: manage move with MoveAction
				break;

			case engine::Asset{ "selectable" }:

				// TODO: manage selection with SelectionAction
				break;

			default:
				debug_printline(0xffffffff, "GUI - unknown trigger action in component: ", jcomponent);
				debug_unreachable();
			}

			gameplay::gamestate::post_add(entity, this->window.name, action);
		}

		Group & load_group(engine::Entity entity, const json & jcomponent)
		{
			debug_assert(contains(jcomponent, "group"));

			const json jgroup = jcomponent["group"];

			return components.emplace<Group>(
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.layout(jgroup));
		}

		Text & load_text(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "text"));

			const json jtext = jcomponent["text"];

			// to compensate for text height somewhat
			View::Margin margin = this->load.margin(jcomponent);
			margin.top += 6;

			return components.emplace<Text>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				margin,
				this->load.size_def_wrap(jcomponent),
				this->load.color(jtext, 0xff000000),
				jtext["display"]);
		}

		PanelC & load_panel(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "panel"));

			const json jpanel = jcomponent["panel"];

			return components.emplace<PanelC>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.color(jpanel),
				selectable);
		}

		PanelT & load_texture(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "texture"));

			const json jtexture = jcomponent["texture"];

			return components.emplace<PanelT>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size(jcomponent),
				jtexture["res"],
				selectable);
		}

		void load_components(Group & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				debug_assert(contains(jcomponent, "type"));

				const auto entity = engine::Entity::create();

				const std::string type = jcomponent["type"];

				View * child;

				if (type == "group")
				{
					auto & group = load_group(entity, jcomponent);
					child = &group;

					load_components(group, jcomponent["components"]);
				}
				else
				{
					const bool selectable = has_action(jcomponent);

					if (type == "panel")
					{
						child = &load_panel(entity, jcomponent, selectable);
					}
					else
					if (type == "texture")
					{
						child = &load_texture(entity, jcomponent, selectable);
					}
					else
					if (type == "text")
					{
						child = &load_text(entity, jcomponent, selectable);
					}
					else
					{
						debug_printline(0xffffffff, "GUI - unknown component type: ", jcomponent);
						debug_unreachable();
					}

					if (selectable)
					{
						load_action(entity, jcomponent);
					}

					if (contains(jcomponent, "name"))
					{
						std::string str = jcomponent["name"];
						debug_assert(str.length() > std::size_t{ 0 });
						lookup.emplace<engine::Entity>(str, entity);
					}
				}

				parent.adopt(child);
			}
		}

	public:

		void setup(const json & jwindow)
		{
			load_components(this->window, jwindow["components"]);
		}
	};

	core::container::Collection<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 10>>
		windows;

	std::vector<Window *> window_stack;

	auto & create_window(ResourceLoader & load, const json & jwindow)
	{
		debug_assert(contains(jwindow, "name"));
		debug_assert(contains(jwindow, "group"));

		const std::string name = jwindow["name"];

		debug_printline(0xffffffff, "GUI - creating window: ", name);

		const engine::Asset asset{ name };

		// TODO: read gravity for window position

		const json jgroup = jwindow["group"];

		auto & window = windows.emplace<Window>(
			asset,
			asset,
			load.size(jwindow),
			load.layout(jgroup),
			load.margin(jwindow));

		window_stack.push_back(&window);

		return window;
	}
}

namespace engine
{
namespace gui
{
	void create()
	{
		std::ifstream file("res/gui.json");

		if (!file.is_open())
		{
			debug_printline(0xffffffff, "GUI - res/gui.json file is missing. No GUI is loaded.");
			return;
		}

		const json jcontent = json::parse(file);

		ResourceLoader resourceLoader(jcontent);

		const auto & jwindows = jcontent["windows"];

		for (const auto & jwindow : jwindows)
		{
			Window & window = create_window(resourceLoader, jwindow);

			// loads the components of the window
			Components(window, resourceLoader).setup(jwindow);

			window.measure_window();
		}
	}

	void destroy()
	{
		// TODO: do something
	}

	void show(engine::Asset window)
	{
		select(window);
		windows.get<Window>(window).show_window();
	}

	void hide(engine::Asset window)
	{
		windows.get<Window>(window).hide_window();
	}

	void toggle(engine::Asset window)
	{
		auto & w = windows.get<Window>(window);

		if (w.isShown()) w.hide_window();
		else w.show_window();
	}

	struct Trigger
	{
		void operator() (const CloseAction & action)
		{
			hide(action.window);
		}

		template<typename T>
		void operator() (const T &)
		{}
	};

	void select(engine::Asset window)
	{
		for (std::size_t i = 0; i < window_stack.size(); i++)
		{
			if (window_stack[i]->name == window)
			{
				window_stack.erase(window_stack.begin() + i);
				window_stack.insert(window_stack.begin(), &windows.get<Window>(window));
				break;
			}
		}

		for (std::size_t i = 0; i < window_stack.size(); i++)
		{
			const int order = static_cast<int>(i);
			window_stack[i]->reorder_window(-order * 10);
		}
	}

	void trigger(engine::Entity entity)
	{
		// TODO: use thread safe queue
		components.call(entity, Trigger{});
	}

	void update(engine::Asset window, engine::gui::Datas && datas)
	{
		// TODO: use thread safe queue
	}

	void update(engine::Asset window, core::maths::Vector3f delta)
	{
		// TODO: use thread safe queue
		windows.get<Window>(window).translate_window(delta);
	}
}
}
