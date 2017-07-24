
#include "views.hpp"

#include <core/container/Collection.hpp>

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
			const value_t left;
			const value_t right;
			const value_t top;
			const value_t bottom;

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
				const TYPE type;
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

				engine::graphics::renderer::add(
					comp.entity,
					engine::graphics::data::ui::PanelC{
						comp.render_matrix(),
						comp.render_size(),
						comp.color,
						comp.selectable });
			}

			for (PanelT & comp : components.get<PanelT>())
			{
				comp.translate(delta);

				engine::graphics::renderer::add(
					comp.entity,
					engine::graphics::data::ui::PanelT{
						comp.render_matrix(),
						comp.render_size(),
						comp.texture,
						comp.selectable });
			}

			for (Text & comp : components.get<Text>())
			{
				comp.translate(delta);

				engine::graphics::renderer::add(
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
				engine::graphics::renderer::remove(comp.entity);
			}

			for (PanelT & comp : components.get<PanelT>())
			{
				engine::graphics::renderer::remove(comp.entity);
			}

			for (Text & comp : components.get<Text>())
			{
				engine::graphics::renderer::remove(comp.entity);
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
				engine::graphics::renderer::update(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}

			for (PanelT & child : components.get<PanelT>())
			{
				child.translate(delta);

				// send position update to renderer
				engine::graphics::renderer::update(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}

			for (Text & child : components.get<Text>())
			{
				child.translate(delta);

				// send position update to renderer
				engine::graphics::renderer::update(
					child.entity,
					engine::graphics::data::ModelviewMatrix{ child.render_matrix() });
			}
		}

		auto & create_panelC(Group & parent, View::Gravity gravity, View::Margin margin, View::Size size, Color color, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelC>(
				entity, entity, gravity, margin, size, color, selectable);

			parent.adopt(&v);
			return v;
		}
		auto & create_panelC(View::Gravity gravity, View::Margin margin, View::Size size, Color color, bool selectable = false)
		{
			return create_panelC(*this, gravity, margin, size, color, selectable);
		}

		auto & create_panelT(Group & parent, View::Gravity gravity, View::Margin margin, View::Size size, engine::Asset texture, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelT>(
				entity, entity, gravity, margin, size, texture, selectable);

			parent.adopt(&v);
			return v;
		}
		auto & create_panelT(View::Gravity gravity, View::Margin margin, View::Size size, engine::Asset texture, bool selectable = false)
		{
			return create_panelT(*this, gravity, margin, size, texture, selectable);
		}

		auto & create_text(Group & parent, View::Gravity gravity, View::Margin margin, Color color, std::string display)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<Text>(
				entity, entity, gravity, margin, View::Size{ 0, 0 }, color, display);

			parent.adopt(&v);
			return v;
		}
		auto & create_text(View::Gravity gravity, View::Margin margin, Color color, std::string display)
		{
			return create_text(*this, gravity, margin, color, display);
		}

		auto & create_group(Group & parent, View::Gravity gravity, View::Margin margin, View::Size size, Group::Layout layout)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<Group>(
				entity, gravity, margin, size, layout);

			parent.adopt(&v);
			return v;
		}
		auto & create_group(View::Gravity gravity, View::Margin margin, View::Size size, Group::Layout layout)
		{
			return create_group(*this, gravity, margin, size, layout);
		}
	};

	core::container::Collection<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 10>>
		windows;

	std::vector<Window *> window_stack;
}

// TODO: this should be solved better.
namespace gameplay
{
namespace gamestate
{
	void post_add(engine::Entity entity, engine::Asset window, engine::Asset name);
}
}

namespace engine
{
namespace gui
{
	void create()
	{
		// create a window
		{
			auto & window = windows.emplace<Window>(
				engine::Asset{ "profile" },
				engine::Asset{ "profile" },
				View::Size{ 220 ,320 },
				Group::Layout::RELATIVE,
				Vector3f{20.f, 40.f, 0.f} );

			window_stack.push_back(&window);

			// main background color
			auto & panelBackground = window.create_panelC(
				View::Gravity{},
				View::Margin{},
				View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
				0xFFFF00FF,
				true);

			gameplay::gamestate::post_add(panelBackground.entity, "profile", "background");

			auto & group = window.create_group(
				View::Gravity{},
				View::Margin{ 10, 10, 10, 10 },
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
				Group::Layout::VERTICAL);

			// add views to the group
			{
				{
					auto & groupHeader = window.create_group(
						group,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::FIXED, 100 } },
						Group::Layout::RELATIVE);

					auto & mover = window.create_panelC(
						groupHeader,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::PARENT } },
							0xFFFF0000,
							true);

					// register the panel as a "mover" for the window
					gameplay::gamestate::post_add(mover.entity, "profile", "mover");

					window.create_text(
						groupHeader,
						View::Gravity{ View::Gravity::HORIZONTAL_LEFT | View::Gravity::VERTICAL_CENTRE },
						View::Margin{ 10 },
						0xFF0000FF,
						"Profile window");
				}
				{
					auto & group2 = window.create_group(
						group,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::PARENT } },
						Group::Layout::VERTICAL);

					window.create_panelT(
						group2,
						View::Gravity{ View::Gravity::HORIZONTAL_CENTRE },
						View::Margin{},
						View::Size{
							{ 100 },
							{ 100 } },
						engine::Asset{ "photo" });

					window.create_panelC(
						group2,
						View::Gravity{ View::Gravity::HORIZONTAL_RIGHT},
						View::Margin{},
						View::Size{
							{ 100 },
							{ View::Size::TYPE::PARENT } },
							0xFF00FFFF);
				}
			}

			// update size and offset of windows components
			window.measure();
		}

		{
			auto & window = windows.emplace<Window>(
				engine::Asset{ "inventory" },
				engine::Asset{ "inventory" },
				View::Size{ 420, 220 },
				Group::Layout::RELATIVE,
				Vector3f{ 250.f, 40.f, 0.f });

			window_stack.push_back(&window);

			// main background color
			auto & panelBackground = window.create_panelC(
				View::Gravity{},
				View::Margin{},
				View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
				0xFF0000FF,
				true);

			gameplay::gamestate::post_add(panelBackground.entity, "inventory", "background");

			auto & groupMain = window.create_group(
				View::Gravity{},
				View::Margin{ 10, 10, 10, 10 },
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
				Group::Layout::HORIZONTAL);

			{
				auto & groupLeft = window.create_group(
					groupMain,
					View::Gravity{},
					View::Margin{},
					View::Size{ 200, View::Size::TYPE::PARENT },
					Group::Layout::VERTICAL);

				for (int i = 0; i < 20; i++)
				{
					uint32_t c = 255 * i / 19;
					window.create_panelC(
						groupLeft,
						View::Gravity{},
						View::Margin{},
						View::Size{ View::Size::TYPE::PARENT, 10 },
						0xFF000000 + c + (c << 8) + (c << 16));
				}
			}
			{
				auto & groupRight = window.create_group(
					groupMain,
					View::Gravity{},
					View::Margin{},
					View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
					Group::Layout::RELATIVE);

				for (int i = 0; i < 10; i++)
				{
					uint32_t c = 255 * i / 9;
					window.create_panelC(
						groupRight,
						View::Gravity{ View::Gravity::HORIZONTAL_CENTRE | View::Gravity::VERTICAL_CENTRE },
						View::Margin{ i*10, i*10, i*10, i*10 },
						View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
						0xFF000000 + c + (c << 8) + (c << 16));
				}
			}

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
