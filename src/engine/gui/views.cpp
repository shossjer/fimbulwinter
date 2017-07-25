
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
	typedef int value_t;
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
				WRAP
			};

			struct Dimen
			{
				TYPE type;
				value_t value;

				Dimen(TYPE type, value_t value)
					: type(type)
					, value(value)
				{
				}

				Dimen(TYPE type)
					: type(type)
					, value(0)
				{
					debug_assert(type != TYPE::FIXED);
				}

				Dimen(value_t value)
					: type(TYPE::FIXED)
					, value(value)
				{
					debug_assert(value >= 0);
				}

				void operator-=(const Dimen other)
				{
					this->value -= other.value;
				}

				void operator-=(const value_t other_value)
				{
					this->value -= other_value;
				}

				bool operator==(const TYPE other_type) const
				{
					return this->type == other_type;
				}

				bool operator >= (const Dimen other) const
				{
					return this->value >= other.value;
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
				offset_vertical = buff[1] + (size_parent.height.value / 2) - (this->size.height.value / 2);
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
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

	private:

		static void measure_dimen(const Size::Dimen parent_size, const value_t margin, Size::Dimen & dimen)
		{
			const value_t max_size = parent_size.value - margin;

			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(max_size >= dimen.value);
				break;
			case Size::TYPE::PARENT:

				dimen.value = max_size;
				break;
			default:
				debug_unreachable();
			}
		}

	public:

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

		void measure(
			const Size size_parent) override
		{
			// measure size
			measure_dimen(size_parent.height, this->margin.height(), this->size.height);
			measure_dimen(size_parent.width, this->margin.width(), this->size.width);
		}

		void translate(core::maths::Vector3f delta)
		{
			this->offset += delta;
		}

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

		static void measure_dimen(const Size::Dimen parent_size, const value_t margin, Size::Dimen & dimen)
		{
			const value_t max_size = parent_size.value - margin;

			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(max_size >= dimen.value);
				break;
			case Size::TYPE::PARENT:
			case Size::TYPE::WRAP:
			default:

				dimen.value = max_size;
				break;
			}
		}

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

					if (this->size.height.type == Size::TYPE::WRAP)
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

		void measure(const Size size_parent) override
		{
			measure_dimen(size_parent.height, this->margin.height(), this->size.height);
			measure_dimen(size_parent.width, this->margin.width(), this->size.width);
			measure_children();
		}
	};

	bool contains(const json jdata, const std::string key)
	{
		return jdata.find(key) != jdata.end();
	}

	engine::graphics::data::Color parse_color(const json & jdata)
	{
		debug_assert(contains(jdata, "color"));

		const std::string str = jdata["color"];
		return std::stoul(str, nullptr, 16);
	}

	engine::graphics::data::Color parse_color(const json & jdata, const engine::graphics::data::Color def_val)
	{
		if (!contains(jdata, "color")) return def_val;

		const std::string str = jdata["color"];
		return std::stoul(str, nullptr, 16);
	}

	Group::Layout parse_layout(const json & jgroup)
	{
		debug_assert(contains(jgroup, "layout"));

		const std::string str = jgroup["layout"];

		if (str == "horizontal")return Group::Layout::HORIZONTAL;
		if (str == "vertical") return Group::Layout::VERTICAL;
		if (str == "relative") return Group::Layout::RELATIVE;

		debug_printline(0xffffffff, "GUI - invalid layout: ", str);
		debug_unreachable();
	}

	View::Size::Dimen parse_dimen(const json jd)
	{
		if (jd.is_string())
		{
			const std::string str = jd;

			if (str == "parent") return View::Size::Dimen{ View::Size::TYPE::PARENT };
			if (str == "wrap") return View::Size::Dimen{ View::Size::TYPE::WRAP };

			debug_printline(0xffffffff, "GUI - invalid size dimen: ", str);
			debug_unreachable();
		}

		return View::Size::Dimen{ (int)jd };
	}

	View::Size parse_size(const json & jgroup)
	{
		debug_assert(contains(jgroup, "size"));

		const json jsize = jgroup["size"];

		debug_assert(contains(jsize, "w"));
		debug_assert(contains(jsize, "h"));

		return View::Size{ parse_dimen(jsize["w"]), parse_dimen(jsize["h"]) };
	}

	View::Gravity parse_gravity(const json & jdata)
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

				if (str == "top") h = View::Gravity::VERTICAL_TOP;
				else
				if (str == "centre") h = View::Gravity::VERTICAL_CENTRE;
				else
				if (str == "bottom") h = View::Gravity::VERTICAL_BOTTOM;
				else
				{
					debug_printline(0xffffffff, "GUI - invalid vertical gravity: ", str);
					debug_unreachable();
				}
			}
		}

		return View::Gravity{ h | v };
	}

	View::Margin parse_margin(const json & jdata)
	{
		View::Margin margin;

		if (contains(jdata, "margin"))
		{
			const json jmargin = jdata["margin"];

			if (contains(jmargin, "b")) margin.bottom = jmargin["b"];
			if (contains(jmargin, "l")) margin.left = jmargin["l"];
			if (contains(jmargin, "r")) margin.right = jmargin["r"];
			if (contains(jmargin, "t")) margin.top = jmargin["t"];
		}

		return margin;
	}

	class Window : private Group
	{
	public:

		engine::Asset name;

	private:

		Vector3f position;

		bool shown;

		// used to determine the drawing order / depth of the window components.
		int order;

		// Components could be moved to global namespace instead since it now has Entity as key.
		// But window still needs to know which "drawable" components it has for when the window
		// is moved.
		// When moved each component needs to have its "position" updated and sent to renderer.
		// Of course we can use the grandparent pointer and go throuh all children of the window,
		// but only the graphical ones are affected... Input needed.
		core::container::Collection
			<
				engine::Entity, 201,
				std::array<Group, 40>,
				std::array<PanelC, 50>,
				std::array<PanelT, 50>,
				std::array<Text, 50>
			>
			components;

	public:

		//Window(Margin margin, Size size) : View(margin, size)
		Window(engine::Asset name, Size size, Layout layout, Vector3f position)
			: Group{ Gravity{}, Margin{}, size, layout }
			, name(name)
			, position(position)
			, shown(false)
			, order(0)
		{
			debug_assert(size.height.type != Size::TYPE::PARENT);
			debug_assert(size.width.type != Size::TYPE::PARENT);
		}

		void measure()
		{
			measure_children();

			float depth = static_cast<float>(this->order);
			arrange_children(this->position, depth);
		}

		void show()
		{
			debug_assert(!this->shown);
			this->shown = true;

			const Vector3f delta{ 0.f, 0.f, static_cast<float>( -(this->order)) };

			// move windows order to first when shown
			this->order = 0;
			this->position += delta;

			for (PanelC & comp : components.get<PanelC>())
			{
				comp.translate(delta);

				engine::graphics::renderer::post_add_panel(
					comp.entity,
					engine::graphics::data::ui::PanelC{
						comp.render_matrix(),
						comp.render_size(),
						comp.color });
				if (comp.selectable)
				{
					engine::graphics::renderer::post_make_selectable(comp.entity);
				}
			}

			for (PanelT & comp : components.get<PanelT>())
			{
				comp.translate(delta);

				engine::graphics::renderer::post_add_panel(
					comp.entity,
					engine::graphics::data::ui::PanelT{
						comp.render_matrix(),
						comp.render_size(),
						comp.texture });
				if (comp.selectable)
				{
					engine::graphics::renderer::post_make_selectable(comp.entity);
				}
			}

			for (Text & comp : components.get<Text>())
			{
				comp.translate(delta);

				engine::graphics::renderer::post_add_text(
					comp.entity,
					engine::graphics::data::ui::Text{
					comp.render_matrix(),
					comp.color,
					comp.display });
			}
		}

		void hide()
		{
			debug_assert(this->shown);
			this->shown = false;

			for (PanelC & comp : components.get<PanelC>())
			{
				engine::graphics::renderer::post_remove(comp.entity);
			}

			for (PanelT & comp : components.get<PanelT>())
			{
				engine::graphics::renderer::post_remove(comp.entity);
			}

			for (Text & comp : components.get<Text>())
			{
				engine::graphics::renderer::post_remove(comp.entity);
			}
		}

		void reorder(const int window_order)
		{
			if (!this->shown) return;
			if (this->order == window_order) return;

			const float delta_depth = window_order - this->order;

			this->order = window_order;

			this->operator=(Vector3f{ 0.f, 0.f, delta_depth });
		}

		void operator = (std::vector<std::pair<engine::Entity, engine::gui::Data>> datas)
		{
			for (auto & data : datas)
			{
				components.update(data.first, data.second);
			}

			measure();
		}

		// the window has been moved, all its drawable components needs to post
		// position updates to renderer.
		void operator = (core::maths::Vector3f delta)
		{
			this->position += delta;

			for (PanelC & child : components.get<PanelC>())
			{
				child.translate(delta);

				// send position update to renderer
				engine::graphics::renderer::post_update_modelviewmatrix(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}

			for (PanelT & child : components.get<PanelT>())
			{
				child.translate(delta);

				// send position update to renderer
				engine::graphics::renderer::post_update_modelviewmatrix(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}

			for (Text & child : components.get<Text>())
			{
				child.translate(delta);

				// send position update to renderer
				engine::graphics::renderer::post_update_modelviewmatrix(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}
		}

		void create_components(Group & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				debug_assert(contains(jcomponent, "type"));

				const auto entity = engine::Entity::create();

				const std::string type = jcomponent["type"];

				if (type == "group")
				{
					debug_assert(contains(jcomponent, "group"));

					const json jgroup = jcomponent["group"];

					auto & group = this->components.emplace<Group>(
						entity,
						parse_gravity(jcomponent),
						parse_margin(jcomponent),
						parse_size(jcomponent),
						parse_layout(jgroup));

					parent.adopt(&group);

					create_components(group, jcomponent["components"]);
				}
				else
				{
					const bool selectable = contains(jcomponent, "action");

					View * child;

					if (type == "panel")
					{
						debug_assert(contains(jcomponent, "panel"));

						const json jpanel = jcomponent["panel"];

						child = &this->components.emplace<PanelC>(
							entity,
							entity,
							parse_gravity(jcomponent),
							parse_margin(jcomponent),
							parse_size(jcomponent),
							parse_color(jpanel),
							selectable);
					}
					else
					if (type == "texture")
					{
						debug_assert(contains(jcomponent, "texture"));

						const json jtexture = jcomponent["texture"];

						child = &this->components.emplace<PanelT>(
							entity,
							entity,
							parse_gravity(jcomponent),
							parse_margin(jcomponent),
							parse_size(jcomponent),
							jtexture["res"],
							selectable);
					}
					else
					if (type == "text")
					{
						debug_assert(contains(jcomponent, "text"));

						const json jtext = jcomponent["text"];

						child = &this->components.emplace<Text>(
							entity,
							entity,
							parse_gravity(jcomponent),
							parse_margin(jcomponent),
							View::Size{ 0, 0 },
							parse_color(jtext, 0xffffffff),
							jtext["display"]);
					}

					parent.adopt(child);

					if (selectable)
					{
						const std::string str = jcomponent["action"];

						// TODO: validate action

						gameplay::gamestate::post_add(entity, this->name, engine::Asset{ str });
					}
				}
			}
		}
		void create_components(const json & jcomponents)
		{
			create_components(*this, jcomponents);
		}
	};

	core::container::Collection<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 10>>
		windows;

	std::vector<Window *> window_stack;

	auto & create_window(const json & jwindow)
	{
		debug_assert(contains(jwindow, "name"));
		debug_assert(contains(jwindow, "group"));

		const std::string name = jwindow["name"];

		debug_printline(0xffffffff, "GUI - creating window: ", name);

		const engine::Asset asset{ name };

		// TODO: read gravity for window position
		const auto margin = parse_margin(jwindow);

		const json jgroup = jwindow["group"];

		auto & window = windows.emplace<Window>(
			asset,
			asset,
			parse_size(jwindow),
			parse_layout(jgroup),
			Vector3f{ static_cast<float>(margin.left), static_cast<float>(margin.top), 0.f });

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
		const auto & jwindows = jcontent["windows"];

		for (const auto & jwindow : jwindows)
		{
			Window & window = create_window(jwindow);

			window.create_components(jwindow["components"]);
			window.measure();
		}
	}

	void destroy()
	{
		// TODO: do something
	}

	void show(engine::Asset window)
	{
		select(window);
		windows.get<Window>(window).show();
	}

	void hide(engine::Asset window)
	{
		windows.get<Window>(window).hide();
	}

	void select(engine::Asset window)
	{
		for (int i = 0; i < window_stack.size(); i++)
		{
			if (window_stack[i]->name == window)
			{
				window_stack.erase(window_stack.begin() + i);
				window_stack.insert(window_stack.begin(), &windows.get<Window>(window));
				break;
			}
		}

		for (int i = 0; i < window_stack.size(); i++)
		{
			window_stack[i]->reorder(-i*10);
		}
	}

	void update(engine::Asset window, std::vector<std::pair<engine::Entity, Data>> datas)
	{
		// TODO: use thread safe queue
		windows.update(window, datas);
	}

	void update(engine::Asset window, core::maths::Vector3f delta)
	{
		// TODO: use thread safe queue
		windows.update(window, delta);
	}
}
}
