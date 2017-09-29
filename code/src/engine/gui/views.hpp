
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_VIEWS_HPP
#define ENGINE_GUI_VIEWS_HPP

#include "gui.hpp"
#include "loading.hpp"
#include "placers.hpp"

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>
#include <engine/graphics/renderer.hpp>

#include <core/container/Collection.hpp>

#include <unordered_map>

namespace engine
{
namespace gui
{
	constexpr float DEPTH_INC = .1f;

	using Color = engine::graphics::data::Color;

	class Lookup
	{
	private:

		std::unordered_map<engine::Asset, engine::Entity> data;

	public:

		void put(engine::Asset name, engine::Entity entity)
		{
			auto r = this->data.emplace(name, entity);
			if (!r.second)
			{
				r.first->second = entity;
			}
		}

		bool contains(engine::Asset name) const
		{
			return this->data.find(name) != this->data.end();
		}

		engine::Entity get(engine::Asset name) const
		{
			return this->data.find(name)->second;
		}
	};

	struct change_t
	{
	private:
		using value_t = uint32_t;

		static constexpr value_t NONE = 0;
		static constexpr value_t VISIBILITY = 1 << 1;
		static constexpr value_t MOVE = 1 << 2;
		static constexpr value_t SIZE = 1 << 3;
		static constexpr value_t INITIAL = 1 << 4;

	private:
		value_t flags;

	public:
		change_t() : flags(INITIAL) {}

	private:
		bool is_set(const value_t flag) const { return (this->flags & flag) != 0; }
		void set(const value_t flags) { this->flags |= flags; };

	public:
		bool any() const { return this->flags != NONE; }

		bool affects_offset() const { return is_set(MOVE); }
		bool affects_size() const { return is_set(SIZE); }

		void clear() { this->flags = NONE; }

		void set_moved() { set(MOVE); }
		void set_resized() { set(MOVE); }
		void set_hidden() { set(MOVE); }
		void set_shown() { set(MOVE); }
		void set_data() { set(MOVE); }

		void set(const change_t other) { set(other.flags); }
	};

	class View
	{
		friend class Group;
		friend class Window;
		friend struct Updater;

	public:

		const engine::Entity entity;
		const engine::Asset name;

		const Gravity gravity;

		enum class State
		{
			DEFAULT,
			HIGHLIGHT,
			PRESSED
		}
		state;

	protected:

		change_t change;

		Margin margin;

		Size size;

		Vector3f offset;

		bool is_invisible;
		bool is_rendered;
		bool should_render;

	protected:

		View(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size)
			: entity(entity)
			, name(name)
			, gravity(gravity)
			, state(State::DEFAULT)
			, change()
			, margin(margin)
			, size(size)
			, offset()
			, is_invisible(false)
			, is_rendered(false)
			, should_render(false)
		{}

	public:

		virtual View * find(const engine::Asset name);
		virtual View * find(const engine::Entity entity);

		// NOTE: could have these as show/hide and "make_invisible", "make_visible"
		// removes the view from renderer
		virtual void renderer_hide();
		// adds the view from renderer
		// only adds if it is "visible"
		virtual void renderer_show();

		// makes the view "hidden", will remove the view from renderer
		// the view cannot be added to renderer while visibly "hidden"
		virtual void visibility_hide();
		// makes the view possible to show in rederer.
		// this will not add the view to renderer!
		virtual void visibility_show();

		virtual void update(const State state) = 0;

		virtual void translate(const Vector3f delta) = 0;

		virtual void translate(unsigned & order) = 0;

	public:

		// total height of the view including margins
		value_t height() const;

		// total width of the view including margins
		value_t width() const;

		Size get_size() const { return this->size; };

	protected:

		// needs to be called after any changes has been made to Components
		virtual change_t refresh1() = 0;

		virtual void refresh2(const Group *const parent) = 0;

		// called after size has been measured
		// the only method to send data to renderer.
		// sends updated drawable view data to renderer if views has changed.
		// sends add, update, remove based on changes made in the view.
		virtual void refresh2(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) = 0;

		Vector3f arrange_offset(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) const;
	};

	class Drawable : public View
	{
	private:

		bool selectable;

		// turned into matrix and sent to renderer
		float order_depth;

	protected:

		Drawable(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size,
			bool selectable)
			: View(entity, name, gravity, margin, size)
			, selectable(selectable)
		{
		}

	protected:

		bool is_selectable() const { return this->selectable; }

		Matrix4x4f render_matrix() const;

		core::maths::Vector2f render_size() const;

		virtual void renderer_send_add() const = 0;
		virtual void renderer_send_update() const = 0;

	public:

		void update(const State state) override;

		void translate(core::maths::Vector3f delta) override;

		void translate(unsigned & order) override;

	protected:

		void refresh2(const Group * const) override;

		void refresh2(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) override;

	private:

		void refresh();
	};

	class PanelC : public Drawable
	{
	public:

		const resource::ColorResource * color;

	public:

		PanelC(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size,
			const resource::ColorResource * color,
			bool selectable)
			: Drawable(entity, name, gravity, margin, size, selectable)
			, color(color)
		{
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

	protected:

		change_t refresh1() override;

		void renderer_send_add() const override;
		void renderer_send_update() const override;
	};

	class PanelT : public Drawable
	{
	public:

		engine::Asset texture;

	public:

		PanelT(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size,
			engine::Asset texture,
			bool selectable)
			: Drawable(entity, name, gravity, margin, size, selectable)
			, texture(texture)
		{
			debug_assert(size.height.type != Size::TYPE::WRAP);
			debug_assert(size.width.type != Size::TYPE::WRAP);
		}

	protected:

		change_t refresh1() override;

		void renderer_send_add() const override;
		void renderer_send_update() const override;
	};

	class Text : public Drawable
	{
	public:

		const resource::ColorResource * color;

		std::string display;

	public:

		Text(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin, Size size,
			const resource::ColorResource * color,
			std::string display)
			: Drawable(entity, name, gravity, margin, size, false)
			, color(color)
			, display(display)
		{
		}

	protected:

		change_t refresh1() override;

		void renderer_send_add() const override;
		void renderer_send_update() const override;
	};

	class Group : public View
	{
		friend class Window;

	public:

		std::vector<View*> children;

		Layout layout;

	public:

		Group(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size,
			Layout layout)
			: View(entity, name, gravity, margin, size)
			, layout(layout)
		{}

	private:

		void arrange_children(Vector3f offset);

	public:

		void adopt(View * child) { this->children.push_back(child); }

		View * find(const engine::Asset name) override;
		View * find(const engine::Entity name) override;

		void renderer_hide() override;

		void renderer_show() override;

		void visibility_hide() override;

		void visibility_show() override;

		void update(const State state) override;

		void translate(const Vector3f delta) override;

		virtual void translate(unsigned & order) override;

	protected:

		change_t refresh1() override;

		void refresh2(const Group *const parent) override;

		void refresh2(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent) override;
	};

	class List : public Group
	{
		friend struct Updater;

	private:

		std::vector<Lookup> lookups;

		const ListData view_template;

		std::size_t shown_items;

		unsigned order;

	public:

		List(
			engine::Entity entity,
			engine::Asset name,
			Gravity gravity,
			Margin margin,
			Size size,
			Layout layout,
			const ListData & view_template)
			: Group(entity, name, gravity, margin, size, layout)
			, view_template(view_template)
			, shown_items(0)
		{}

	public:

		void translate(unsigned & order) override;
	};

	class Window
	{
	public:

		Group group;

	private:

		Vector3f position;

		// used to determine the drawing order / depth of the window components.
		int order;

		bool dirty;
		bool shown;

	public:

		Window(engine::Asset name, Size size, Layout layout, Margin margin)
			: group{ engine::Entity::create(), name, Gravity{}, Margin{}, size, layout }
			, position({ static_cast<float>(margin.left), static_cast<float>(margin.top), 0.f })
			, order(0)
			, dirty(false)
			, shown(false)
		{
			debug_assert(size.height.type != Size::TYPE::PARENT);
			debug_assert(size.width.type != Size::TYPE::PARENT);
		}

		engine::Asset name() const { return this->group.name; }

		bool is_dirty() const { return this->dirty; }
		bool is_shown() const { return this->shown; }

		void splash_mud() { this->dirty = true; }

		void refresh_window();

		void show_window();

		void hide_window();

		void init_window();

		void reorder_window(const int window_order);

		void translate_window(const Vector3f delta);
	};
}
}

#endif // !ENGINE_GUI_COMPONENTS_HPP
