
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_VIEWS_HPP
#define ENGINE_GUI_VIEWS_HPP

#include "gui.hpp"
#include "loading.hpp"
#include "placers.hpp"

#include <engine/common.hpp>
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
		// TODO: make map
		core::container::Collection
			<
			engine::Asset,
			201,
			std::array<engine::Entity, 100>
			> data;

	public:

		void put(engine::Asset name, engine::Entity entity)
		{
			this->data.emplace<engine::Entity>(name, entity);
		}

		bool contains(engine::Asset name)
		{
			return this->data.contains(name);
		}

		engine::Entity get(engine::Asset name)
		{
			return this->data.get<engine::Entity>(name);
		}
	};

	class View
	{
		friend struct Updater;
		friend class Window;

	public:

		const Gravity gravity;

	protected:

		const Margin margin;

		Size size;

		bool shown;

	protected:

		View(
			Gravity gravity,
			Margin margin,
			Size size)
			: gravity(gravity)
			, margin(margin)
			, size(size)
			, shown(false)
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

		void refresh() override;
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

		void refresh() override;
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

		void refresh() override;
	};

	class Group : public View
	{
		friend class Window;

	protected:

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

		List(Gravity gravity, Margin margin, Size size, Layout layout, const ListData & view_template)
			: Group(gravity, margin, size, layout)
			, view_template(view_template)
			, shown_items(0)
		{}
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

		Window(engine::Asset name, Size size, Layout layout, Margin margin)
			: name(name)
			, group{ Gravity{}, Margin{}, size, layout }
			, position({ static_cast<float>(margin.left), static_cast<float>(margin.top), 0.f })
			, shown(false)
			, order(0)
		{
			debug_assert(size.height.type != Size::TYPE::PARENT);
			debug_assert(size.width.type != Size::TYPE::PARENT);
		}

		bool is_shown() const { return this->shown; }

		void show_window();

		void hide_window();

		void measure_window();

		void update_window();

		void reorder_window(const int window_order);

		void translate_window(const Vector3f delta);
	};
}
}

#endif // !ENGINE_GUI_COMPONENTS_HPP
