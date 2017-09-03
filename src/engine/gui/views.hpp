
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
			this->data.insert_or_assign(name, entity);
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

	class View
	{
		friend struct Updater;
		friend class Window;

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

		const Margin margin;

		Size size;

		bool is_dirty;
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
			, margin(margin)
			, size(size)
			, is_dirty(false)
			, is_invisible(false)
			, is_rendered(false)
			, should_render(false)
		{}

	public:

		// called after size has been measured
		virtual void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) = 0;

		virtual View * find(const engine::Asset name);
		virtual View * find(const engine::Entity entity);

		// needs to be called after any changes has been made to Components
		virtual void measure(const Size size_parent) = 0;

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

		//// sets view (and children if group) should_render = true
		//virtual void show();

		//// sets view (and children if group) should_render = false
		//virtual void hide();

		virtual void update(const State state) = 0;

		// the only method to send data to renderer.
		// sends updated drawable view data to renderer if views has changed.
		// sends add, update, remove based on changes made in the view.
		virtual void refresh() = 0;

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
	private:

		bool selectable;

		// turned into matrix and sent to renderer
		Vector3f offset;

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

		void update_translation(core::maths::Vector3f delta);

		Matrix4x4f render_matrix() const;

		core::maths::Vector2f render_size() const;

		virtual void renderer_send_add() const = 0;
		virtual void renderer_send_update() const = 0;

	public:

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override;

		void refresh() override;

		void update(const State state) override;

		void translate(core::maths::Vector3f delta) override;
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

		void measure(const Size parent) override;

	protected:

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

		void measure(const Size parent) override;

	protected:

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

		void measure(const Size parent) override;

	protected:

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

		void measure_children();

		void arrange_children(Vector3f offset, float & offset_depth);

	public:

		void adopt(View * child) { this->children.push_back(child); }

		void arrange(
			const Size size_parent,
			const Gravity gravity_mask_parent,
			const Vector3f offset_parent,
			float & offset_depth) override;

		View * find(const engine::Asset name) override;
		View * find(const engine::Entity name) override;

		void measure(const Size parent) override;

		void renderer_hide() override;

		void renderer_show() override;

		void visibility_hide() override;

		void visibility_show() override;

		void update(const State state) override;

		void refresh() override;

		void translate(const Vector3f delta) override;
	};

	class List : public Group
	{
		friend struct Updater;

	private:

		std::vector<Lookup> lookups;

		const ListData view_template;

		std::size_t shown_items;

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

		void reorder_window(const int window_order);

		void translate_window(const Vector3f delta);

	private:

		void measure_window();
	};
}
}

#endif // !ENGINE_GUI_COMPONENTS_HPP
