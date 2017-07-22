
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
			const Margin margin_parent,
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
				offset_horizontal = buff[0] + size_parent.width.value - this->size.width.value;
			}
			else
			if (this->gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_CENTRE))
			{
				offset_horizontal = buff[0] + (size_parent.width.value / 2) - (this->size.width.value / 2);
			}
			else // LEFT default
			{
				offset_horizontal = buff[0];
			}

			float offset_vertical;

			if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_BOTTOM))
			{
				offset_vertical = buff[1] + size_parent.height.value - this->size.height.value;
			}
			else
			if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_CENTRE))
			{
				offset_vertical = buff[1] + (size_parent.height.value / 2) - (this->size.height.value / 2);
			}
			else // TOP default
			{
				offset_vertical = buff[1];
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

		static void measure_dimen(const value_t parent_margin, const Size::Dimen parent_size, Size::Dimen & dimen)
		{
			const value_t parent_total = parent_size.value - parent_margin;

			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(parent_total >= dimen.value);
				break;
			case Size::TYPE::PARENT:

				dimen.value = parent_total;
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
			const Margin margin_parent,
			const Size size_parent) override
		{
			// measure size
			measure_dimen(margin_parent.height(), size_parent.height, this->size.height);
			measure_dimen(margin_parent.width(), size_parent.width, this->size.width);
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

		static void measure_dimen(const value_t parent_margin, const Size::Dimen parent_size, Size::Dimen & dimen)
		{
			const value_t parent_total = parent_size.value - parent_margin;

			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(parent_total >= dimen.value);
				break;
			case Size::TYPE::PARENT:
			case Size::TYPE::WRAP:
			default:

				dimen.value = parent_total;
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
					p->arranage(size_parent, mask, offset, offset_depth);
					offset += Vector3f(static_cast<float>(p->width()), 0.f, 0.f);
				}
			}
			else
			{
				const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

				for (auto p : this->children)
				{
					p->arranage(size_parent, mask, offset, offset_depth);
					offset += Vector3f(0.f, static_cast<float>(p->height()), 0.f);
				}
			}
		}

		void measure(
			const Margin margin_parent,
			const Size size_parent) override
		{
			measure_dimen(margin_parent.height(), size_parent.height, this->size.height);
			measure_dimen(margin_parent.width(), size_parent.width, this->size.width);

			Size size_remaining = this->size;

			if (this->orientation == ORIENTATION::HORIZONTAL)
			{
				for (auto p : this->children)
				{
					p->measure(this->margin, size_remaining);
					size_remaining.width -= p->width();
				}

				if (this->size.width == Size::TYPE::WRAP)
					this->size.width -= size_remaining.width;
			}
			else
			{
				for (auto p : this->children)
				{
					p->measure(this->margin, size_remaining);
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

			for (auto p : this->children)
			{
				p->arranage(size_parent, Gravity::unmasked(), offset, offset_depth);
			}
		}

		void measure(
			const Margin margin_parent,
			const Size size_parent) override
		{
			measure_dimen(margin_parent.height(), size_parent.height, this->size.height);
			measure_dimen(margin_parent.width(), size_parent.width, this->size.width);

			Size size_max = this->size;

			for (auto p : this->children)
			{
				p->measure(this->margin, this->size);

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
		View * grandparent;

		View::Size size;
		Vector3f position;

		bool shown;

		// Components could be moved to global namespace instead since it now has Entity as key.
		// But window still needs to know which "drawable" components it has for when the window
		// is moved.
		// When moved each component needs to have its "position" updated and sent to renderer.
		// Of course we can use the grandparent pointer and go throuh all children of the window,
		// but only the graphical ones are affected... Input needed.
		core::container::Collection<
			engine::Entity, 201,
			std::array<LinearGroup, 100>,
			std::array<PanelC, 100>>
			components;

	public:

		//Window(Margin margin, Size size) : View(margin, size)
		Window(View::Size size, Vector3f position)
			: grandparent(nullptr)
			, size(size)
			, position(position)
			, shown(false)
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
			// TODO: window depth/priority
			float depth = 0.f;

			grandparent->measure(View::Margin{ }, this->size);
			grandparent->arranage(this->size, View::Gravity{}, this->position, depth);
		}

		void show()
		{
			debug_assert(!this->shown);
			this->shown = true;

			for (PanelC & comp : components.get<PanelC>())
			{
				engine::graphics::renderer::add(
					comp.entity,
					engine::graphics::data::ui::PanelC{
						comp.render_matrix(),
						comp.render_size(),
						comp.color,
						comp.selectable });
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
		}

		auto & create_panel(Parent & parent, View::Gravity gravity, View::Margin margin, View::Size size, Color color, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelC>(
				entity, entity, gravity, margin, size, color, selectable);

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
	};

	core::container::Collection<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 10>>
		windows;
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
				View::Size{
					{ View::Size::TYPE::FIXED, 200 },
					{ View::Size::TYPE::FIXED, 300 } },
				Vector3f{20.f, 40.f, 0.f} );

			auto & group = window.create_linear(
				window,
				View::Gravity{},
				View::Margin{},
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
				LinearGroup::ORIENTATION::VERTICAL);

			// add views to the group
			{
				auto & mover = window.create_panel(
					group,
					View::Gravity{},
					View::Margin{},
					View::Size{
						{ View::Size::TYPE::PARENT },
						{ View::Size::TYPE::FIXED, 100 } },
					0xFFFF0000,
					true);

				// register the panel as a "mover" for the window
				gameplay::gamestate::post_add(mover.entity, "profile", "mover");

				{
					auto & group2 = window.create_linear(
						group,
						View::Gravity{},
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::PARENT } },
							LinearGroup::ORIENTATION::VERTICAL);

					window.create_panel(
						group2,
						View::Gravity{ View::Gravity::HORIZONTAL_CENTRE },
						View::Margin{},
						View::Size{
							{ 100 },
							{ 100 } },
							0xFF00FF00);

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
	}

	void show(engine::Asset window)
	{
		windows.get<Window>(window).show();
	}

	void hide(engine::Asset window)
	{
		windows.get<Window>(window).hide();
	}

	void destroy()
	{
		// TODO: do something
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
