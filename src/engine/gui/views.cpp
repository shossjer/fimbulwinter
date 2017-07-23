
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

		const engine::Entity entity;

		const Gravity gravity;

	protected:

		const Margin margin;

		Size size;

	protected:

		View(
			engine::Entity entity,
			Gravity gravity,
			Margin margin,
			Size size)
			: entity(entity)
			, gravity(gravity)
			, margin(margin)
			, size(size)
		{}

	public:

		// called after size has been measured
		virtual void arranage(
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
		Vector3f arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent)
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

		bool selectable;

	protected:

		Drawable(engine::Entity entity, Gravity gravity, Margin margin, Size size, bool selectable)
			: View(entity, gravity, margin, size)
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

		void arranage(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override
		{
			const Vector3f ret = arrange(size_parent, gravity_mask_parent, offset_parent);
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

	// TODO: class PanelT : public Drawable

	// TODO: class Text : public Drawable

	class Parent
	{
	public:
		virtual void adopt(View * child) = 0;
	};

	class Group : public View, public Parent
	{
	protected:

		std::vector<View*> children;

	protected:

		Group(engine::Entity entity, Gravity gravity, Margin margin, Size size)
			: View(entity, gravity, margin, size)
		{}

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

	public:

		void adopt(View * child) override
		{
			this->children.push_back(child);
		}
	};

	class LinearGroup : public Group
	{
	public:

		enum class ORIENTATION
		{
			HORIZONTAL,
			VERTICAL
		};

	private:

		const ORIENTATION orientation;

	public:

		LinearGroup(engine::Entity entity, Gravity gravity, Margin margin, Size size, ORIENTATION orientation)
			: Group(entity, gravity, margin, size)
			, orientation(orientation)
		{}

	public:

		void arranage(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override
		{
			Vector3f offset = arrange(size_parent, gravity_mask_parent, offset_parent);

			if (this->orientation == ORIENTATION::HORIZONTAL)
			{
				const Gravity mask{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

				for (auto p : this->children)
				{
					p->arranage(this->size, mask, offset, offset_depth);
					offset += Vector3f(static_cast<float>(p->width()), 0.f, 0.f);
				}
			}
			else
			{
				const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

				for (auto p : this->children)
				{
					p->arranage(this->size, mask, offset, offset_depth);
					offset += Vector3f(0.f, static_cast<float>(p->height()), 0.f);
				}
			}
		}

		void measure(
			const Size size_parent) override
		{
			measure_dimen(size_parent.height, this->margin.height(), this->size.height);
			measure_dimen(size_parent.width, margin.width(), this->size.width);

			Size size_remaining = this->size;

			if (this->orientation == ORIENTATION::HORIZONTAL)
			{
				for (auto p : this->children)
				{
					p->measure(size_remaining);
					size_remaining.width -= p->width();
				}

				if (this->size.width == Size::TYPE::WRAP)
					this->size.width -= size_remaining.width;
			}
			else
			{
				for (auto p : this->children)
				{
					p->measure(size_remaining);
					size_remaining.height -= p->height();
				}

				if (this->size.height.type == Size::TYPE::WRAP)
					this->size.height -= size_remaining.height;
			}
		}
	};

	class RelativeGroup : public Group
	{
	public:

		RelativeGroup(engine::Entity entity, Gravity gravity, Margin margin, Size size)
			: Group(entity, gravity, margin, size)
		{}

	public:

		void arranage(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override
		{
			const Vector3f offset = arrange(size_parent, gravity_mask_parent, offset_parent);
			const Gravity gravity = Gravity::unmasked();

			for (auto p : this->children)
			{
				p->arranage(this->size, gravity, offset, offset_depth);
			}
		}

		void measure(
			const Size size_parent) override
		{
			measure_dimen(size_parent.height, this->margin.height(), this->size.height);
			measure_dimen(size_parent.width, this->margin.width(), this->size.width);

			Size size_max = this->size;

			for (auto p : this->children)
			{
				p->measure(this->size);

				// used if this size is wrap
				size_max.width.value = std::max(size_max.width.value, p->width());
				size_max.height.value = std::max(size_max.height.value, p->width());
			}

			if (this->size.width == Size::TYPE::WRAP)
				this->size.width -= size_max.width;

			if (this->size.height == Size::TYPE::WRAP)
				this->size.height -= size_max.height;
		}
	};

	class Window : public Parent
	{
	public:

		engine::Asset name;

	private:

		View * grandparent;

		View::Size size;
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
				std::array<LinearGroup, 20>,
				std::array<RelativeGroup, 20>,
				std::array<PanelC, 50>,
				std::array<PanelT, 50>,
				std::array<Text, 50>
			>
			components;

	public:

		//Window(Margin margin, Size size) : View(margin, size)
		Window(engine::Asset name, View::Size size, Vector3f position)
			: name(name)
			, grandparent(nullptr)
			, size(size)
			, position(position)
			, shown(false)
			, order(0)
		{
			debug_assert(size.height.type != View::Size::TYPE::PARENT);
			debug_assert(size.width.type != View::Size::TYPE::PARENT);
		}

		void adopt(View * child) override
		{
			this->grandparent = child;
		}

		void measure()
		{
			grandparent->measure(this->size);

			float depth = static_cast<float>(this->order);
			grandparent->arranage(this->size, View::Gravity{}, this->position, depth);
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

			// TODO: PanelT

			// TODO: Text
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

		auto & create_panel(Parent & parent, View::Gravity gravity, View::Margin margin, View::Size size, Color color, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelC>(
				entity, entity, gravity, margin, size, color, selectable);

			parent.adopt(&v);
			return v;
		}

		auto & create_panelT(Parent & parent, View::Gravity gravity, View::Margin margin, View::Size size, engine::Asset texture, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelT>(
				entity, entity, gravity, margin, size, texture, selectable);

			parent.adopt(&v);
			return v;
		}

		auto & create_text(Parent & parent, View::Gravity gravity, View::Margin margin, Color color, std::string display)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<Text>(
				entity, entity, gravity, margin, View::Size{ 0, 0 }, color, display);

			parent.adopt(&v);
			return v;
		}

		auto & create_linear(Parent & parent, View::Gravity gravity, View::Margin margin, View::Size size, LinearGroup::ORIENTATION orientation)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<LinearGroup>(
				entity, entity, gravity, margin, size, orientation);

			parent.adopt(&v);
			return v;
		}

		auto & create_relative(Parent & parent, View::Gravity gravity, View::Margin margin, View::Size size)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<RelativeGroup>(
				entity, entity, gravity, margin, size);

			parent.adopt(&v);
			return v;
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
				View::Size{
					{ View::Size::TYPE::FIXED, 220 },
					{ View::Size::TYPE::FIXED, 320 } },
				Vector3f{20.f, 40.f, 0.f} );

			window_stack.push_back(&window);

			auto & groupBackground = window.create_relative(
				window,
				View::Gravity{},
				View::Margin{},
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } });

			// main background color
			auto & panelBackground = window.create_panel(
				groupBackground,
				View::Gravity{},
				View::Margin{},
				View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
				0xFFFF00FF,
				true);

			gameplay::gamestate::post_add(panelBackground.entity, "profile", "background");

			auto & group = window.create_linear(
				groupBackground,
				View::Gravity{},
				View::Margin{ 10, 10, 10, 10 },
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
				LinearGroup::ORIENTATION::VERTICAL);

			// add views to the group
			{
				{
					auto & groupHeader = window.create_relative(
						group,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::FIXED, 100 } });

					auto & mover = window.create_panel(
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
					auto & group2 = window.create_linear(
						group,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::PARENT } },
							LinearGroup::ORIENTATION::VERTICAL);

					window.create_panelT(
						group2,
						View::Gravity{ View::Gravity::HORIZONTAL_CENTRE },
						View::Margin{},
						View::Size{
							{ 100 },
							{ 100 } },
						engine::Asset{ "photo" });

					window.create_panel(
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
				View::Size{
					{ View::Size::TYPE::FIXED, 420 },
					{ View::Size::TYPE::FIXED, 220 } },
					Vector3f{ 250.f, 40.f, 0.f });

			window_stack.push_back(&window);

			auto & groupBackground = window.create_relative(
				window,
				View::Gravity{},
				View::Margin{},
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } });

			// main background color
			auto & panelBackground = window.create_panel(
				groupBackground,
				View::Gravity{},
				View::Margin{},
				View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT },
				0xFF0000FF,
				true);

			gameplay::gamestate::post_add(panelBackground.entity, "inventory", "background");

			auto & groupMain = window.create_linear(
				groupBackground,
				View::Gravity{},
				View::Margin{ 10, 10, 10, 10 },
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
					LinearGroup::ORIENTATION::HORIZONTAL);

			{
				auto & groupLeft = window.create_linear(
					groupMain,
					View::Gravity{},
					View::Margin{},
					View::Size{ 200, View::Size::TYPE::PARENT },
					LinearGroup::ORIENTATION::VERTICAL);

				for (int i = 0; i < 20; i++)
				{
					uint32_t c = 255 * i / 19;
					window.create_panel(
						groupLeft,
						View::Gravity{},
						View::Margin{},
						View::Size{ View::Size::TYPE::PARENT, 10 },
						0xFF000000 + c + (c << 8) + (c << 16));
				}
			}
			{
				auto & groupRight = window.create_relative(
					groupMain,
					View::Gravity{},
					View::Margin{},
					View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT });

				for (int i = 0; i < 10; i++)
				{
					uint32_t c = 255 * i / 9;
					window.create_panel(
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
