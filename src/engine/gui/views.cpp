
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
			};

			Dimen width;
			Dimen height;
		};

	public:

		const engine::Entity entity;

	protected:

		const Margin margin;

		Size size;

	protected:

		View(
			engine::Entity entity,
			Margin margin,
			Size size)
			: entity(entity)
			, margin(margin)
			, size(size)
		{}

	public:
		// needs to be called after any changes has been made to Components
		virtual void measure(
			float & offset_depth,
			const float offset_left,
			const float offset_top,
			const Size & size) = 0;

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
	};

	class Drawable : public View
	{
		// turned into matrix and sent to renderer
		Vector3f offset;

	public:

		bool selectable;

	protected:

		Drawable(engine::Entity entity, Margin margin, Size size, bool selectable)
			: View(entity, margin, size)
			, selectable(selectable)
		{
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

	private:

		static void measure_dimen(const Size::Dimen parent, Size::Dimen & dimen)
		{
			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(parent.value >= dimen.value);
				break;
			case Size::TYPE::PARENT:

				dimen.value = parent.value;
				break;
			}
		}

	public:

		void measure(
			float & offset_depth,
			const float offset_left,
			const float offset_top,
			const Size & parent_size) override
		{
			offset_depth += DEPTH_INC;
			const float ol = offset_left + this->margin.left;
			const float ot = offset_top + this->margin.top;

			this->offset = Vector3f{ ol, ot, offset_depth };

			measure_dimen(parent_size.height, this->size.height);
			measure_dimen(parent_size.width, this->size.width);
		}

		void translate(core::maths::Vector3f translation)
		{
			this->offset += translation;
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

		PanelC(engine::Entity entity, Margin margin, Size size, Color color, bool selectable)
			: Drawable(entity, margin, size, selectable)
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

		Group(engine::Entity entity, Margin margin, Size size)
			: View(entity, margin, size)
		{}

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

		LinearGroup(engine::Entity entity, Margin margin, Size size, ORIENTATION orientation)
			: Group(entity, margin, size)
			, orientation(orientation)
		{}

	private:

		static void measure_dimen(const Size::Dimen parent, Size::Dimen & dimen)
		{
			switch (dimen.type)
			{
			case Size::TYPE::FIXED:

				debug_assert(parent.value >= dimen.value);
				break;
			case Size::TYPE::PARENT:
			case Size::TYPE::WRAP:
			default:

				dimen.value = parent.value;
				break;
			}
		}

	public:

		void measure(
			float & offset_depth,
			const float offset_left,
			const float offset_top,
			const Size & size) override
		{
			offset_depth += DEPTH_INC;

			float ol = offset_left + this->margin.left;
			float ot = offset_top + this->margin.top;

			measure_dimen(size.height, this->size.height);
			measure_dimen(size.width, this->size.width);

			Size size_remaining = this->size;
			size_remaining.height.value -= this->margin.height();
			size_remaining.width.value -= this->margin.width();

			for (auto p : this->children)
			{
				p->measure(offset_depth, ol, ot, size_remaining);

				if (this->orientation == ORIENTATION::HORIZONTAL)
				{
					const value_t w = p->width();
					ol += w;
					size_remaining.width.value -= w;
				}
				else
				{
					const value_t h = p->height();
					ot += h;
					size_remaining.height.value -= h;
				}
			}
		}

		// TODO: class RelativeGroup : public Group
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
			core::maths::Vector3f::array_type off;
			this->position.get_aligned(off);

			grandparent->measure(off[2], off[0], off[1], this->size);
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

		auto & create_panel(Parent & parent, View::Margin margin, View::Size size, Color color, bool selectable = false)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<PanelC>(
				entity, PanelC{ entity, margin, size, color, selectable });

			parent.adopt(&v);
			return v;
		}

		auto & create_linear(Parent & parent, View::Margin margin, View::Size size, LinearGroup::ORIENTATION orientation)
		{
			auto entity = engine::Entity::create();

			auto & v = this->components.emplace<LinearGroup>(
				entity, LinearGroup{ entity, margin, size, orientation });

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
				Window{ View::Size{
					{ View::Size::TYPE::FIXED, 200 },
					{ View::Size::TYPE::FIXED, 200 } },
					Vector3f{20.f, 40.f, 0.f} });

			auto & group = window.create_linear(
				window,
				View::Margin{},
				View::Size{
					{ View::Size::TYPE::PARENT },
					{ View::Size::TYPE::PARENT } },
				LinearGroup::ORIENTATION::VERTICAL);

			// add views to the group
			{
				auto & mover = window.create_panel(
					group,
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
						View::Margin{ 20, 20, 20 },
						View::Size{
							{ View::Size::TYPE::PARENT },
							{ View::Size::TYPE::PARENT } },
							LinearGroup::ORIENTATION::HORIZONTAL);

					window.create_panel(
						group2,
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::FIXED, 50 },
							{ View::Size::TYPE::PARENT } },
							0xFF00FF00);

					window.create_panel(
						group2,
						View::Margin{},
						View::Size{
							{ View::Size::TYPE::PARENT },
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
