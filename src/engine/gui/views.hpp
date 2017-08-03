
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_VIEWS_HPP
#define ENGINE_GUI_VIEWS_HPP

#include <engine/common.hpp>
#include <engine/graphics/renderer.hpp>

namespace engine
{
namespace gui
{
	typedef float value_t;
	typedef engine::graphics::data::Color Color;

	constexpr float DEPTH_INC = .1f;

	class View
	{
		friend class Window;

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
		virtual void measure(const Size size_parent) = 0;

		virtual void show(const Vector3f delta) = 0;

		virtual void hide() = 0;

		virtual void translate(const Vector3f delta) = 0;

		// total height of the view including margins
		value_t height() const;

		// total width of the view including margins
		value_t width() const;

	protected:

		Vector3f arrange_offset(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) const;
	};

	class Drawable : public View
	{
	public:

		engine::Entity entity;

	private:

		bool selectable;

		// turned into matrix and sent to renderer
		Vector3f offset;

	protected:

		Drawable(engine::Entity entity, Gravity gravity, Margin margin, Size size, bool selectable)
			: View(gravity, margin, size)
			, entity(entity)
			, selectable(selectable)
		{
		}

	protected:

		bool is_selectable() const { return this->selectable; }

		void update_translation(core::maths::Vector3f delta);

		auto render_matrix() const;

		auto render_size() const;

	public:

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override;

		void hide() override;

		void translate(core::maths::Vector3f delta) override;
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

		void measure(const Size parent) override;

		void show(const Vector3f delta) override;
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

		void measure(const Size parent) override;

		void show(const Vector3f delta) override;
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

		void measure(const Size parent) override;

		void show(const Vector3f delta) override;
	};

	class Group : public View
	{
		friend class Window;

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

	private:

		void measure_children();

		void arrange_children(Vector3f offset, float & offset_depth);

	public:

		void adopt(View * child) { this->children.push_back(child); }

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override;

		void measure(const Size parent) override;

		void show(const Vector3f delta) override;

		void hide() override;

		void translate(const Vector3f delta) override;
	};

	class Window
	{
	public:

		engine::Asset name;

		Group group;

	private:

		Vector3f position;

		bool shown;

		// used to determine the drawing order / depth of the window components.
		int order;

	public:

		Window(engine::Asset name, View::Size size, Group::Layout layout, View::Margin margin)
			: name(name)
			, group{ View::Gravity{}, View::Margin{}, size, layout }
			, position({ static_cast<float>(margin.left), static_cast<float>(margin.top), 0.f })
			, shown(false)
			, order(0)
		{
			debug_assert(size.height.type != View::Size::TYPE::PARENT);
			debug_assert(size.width.type != View::Size::TYPE::PARENT);
		}

		bool is_shown() const { return this->shown; }

		void show_window();

		void hide_window();

		void measure_window();

		void translate_window(const Vector3f delta);

		void reorder_window(const int window_order);
	};
}
}

#endif // !ENGINE_GUI_COMPONENTS_HPP
