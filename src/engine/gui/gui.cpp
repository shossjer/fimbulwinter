
#include "actions.hpp"
#include "functions.hpp"
#include "gui.hpp"
#include "loading.hpp"
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
	using namespace engine::gui;

	core::container::Collection
		<
			engine::Entity, 61,
			std::array<CloseAction, 10>,
			std::array<MoveAction, 10>,
			std::array<SelectAction, 10>
		>
		actions;

	core::container::Collection
		<
			engine::Entity, 201,
			// Views
			std::array<engine::gui::Group, 40>,
			std::array<engine::gui::PanelC, 50>,
			std::array<engine::gui::PanelT, 50>,
			std::array<engine::gui::Text, 50>,
			// Functionality
			std::array<engine::gui::ProgressBar, 50>
		>
		components;

	core::container::Collection
		<
			engine::Asset, 21,
			std::array<engine::gui::Window, 10>,
			std::array<int, 10>
		>
		windows;

	// lookup table for "named" components (asset -> entity)
	// used when gamestate requests updates of components.
	core::container::Collection
		<
			engine::Asset, 201,
			std::array<engine::Entity, 100>,
			std::array<int, 1>
		>
		lookup;

	std::vector<Window *> window_stack;

	struct Creator
	{
		Window & window;

		Creator(Window & window)
			: window(window)
		{}

	private:

		void create_action(const Drawable & drawable, const ViewData & data)
		{
			if (!data.has_action()) return;

			switch (data.action.type)
			{
			case ViewData::Action::CLOSE:

				actions.emplace<engine::gui::CloseAction>(drawable.entity, engine::gui::CloseAction{ this->window.name });
				break;

			case ViewData::Action::MOVER:

				actions.emplace<engine::gui::MoveAction>(drawable.entity, engine::gui::MoveAction{ this->window.name });
				break;

			case ViewData::Action::SELECT:

				actions.emplace<engine::gui::SelectAction>(drawable.entity, engine::gui::SelectAction{ this->window.name });
				break;
			}

			gameplay::gamestate::post_add(drawable.entity, this->window.name, data.action.type);
		}

		void create_function(View & view, const ViewData & data)
		{
			switch (data.function.type)
			{
			case ViewData::Function::PROGRESS:
			{
				const auto entity = engine::Entity::create();

				components.emplace<ProgressBar>(
					entity,
					this->window.name,
					&view,
					data.function.direction);

				lookup.emplace<engine::Entity>(data.function.name, entity);
				break;
			}
			default:
				break;
			}
		}

	public:

		View & operator() (GroupData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<Group>(
				entity,
				data.gravity,
				data.margin,
				data.size,
				data.layout);
			create_views(view, data);
			create_function(view, data);

			return view;
		}

		View & operator() (PanelData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<PanelC>(
				entity,
				entity,
				data.gravity,
				data.margin,
				data.size,
				data.color,
				data.has_action());
			create_action(view, data);
			create_function(view, data);

			if (data.has_name()) lookup.emplace<engine::Entity>(data.name, entity);

			return view;
		}

		View & operator() (TextData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<Text>(
				entity,
				entity,
				data.gravity,
				data.margin,
				data.size,
				data.color,
				data.display);
			create_action(view, data);
			create_function(view, data);

			if (data.has_name()) lookup.emplace<engine::Entity>(data.name, entity);

			return view;
		}

		View & operator() (TextureData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<PanelT>(
				entity,
				entity,
				data.gravity,
				data.margin,
				data.size,
				data.res,
				data.has_action());
			create_action(view, data);

			if (data.has_name()) lookup.emplace<engine::Entity>(data.name, entity);

			return view;
		}

		void create_views(Group & parent, GroupData & dataGroup)
		{
			for (auto & data : dataGroup.children)
			{
				View & view = visit(*this, data);

				parent.adopt(&view);
			}
		}
	};
}

namespace engine
{
namespace gui
{
	void load(std::vector<DataVariant> & windows);

	void create()
	{
		std::vector<DataVariant> windowDatas;

		// load windows and views data from somewhere.
		load(windowDatas);

		window_stack.reserve(windowDatas.size());

		for (auto & windowData : windowDatas)
		{
			GroupData data = utility::get<GroupData>(windowData);

			auto & window = windows.emplace<Window>(
				data.name,
				data.name,
				data.size,
				data.layout,
				data.margin);

			window_stack.push_back(&window);

			Creator(window).create_views(window.group, data);
		}
	}

	void destroy()
	{
		// TODO: destroy something
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

		if (w.is_shown()) w.hide_window();
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
		debug_assert(actions.contains(entity));

		// TODO: use thread safe queue
		actions.call(entity, Trigger{});
	}

	struct Updater
	{
		Data data;

		void operator() (PanelC & panel)
		{
			switch (this->data.type)
			{
			case Data::COLOR:

				panel.color = data.color;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for color panel.");
				debug_unreachable();
			}
		}

		void operator() (PanelT & panel)
		{
			switch (this->data.type)
			{
			case Data::TEXTURE:

				panel.texture = data.texture;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for texture panel.");
				debug_unreachable();
			}
		}

		void operator() (Text & text)
		{
			switch (this->data.type)
			{
			case Data::COLOR:

				text.color = data.color;
				break;

			case Data::DISPLAY:

				text.display = data.display;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for text.");
				debug_unreachable();
			}
		}

		void operator() (ProgressBar & pb)
		{
			switch (this->data.type)
			{
			case Data::PROGRESS:

				if (pb.direction == ProgressBar::HORIZONTAL)
				{
					debug_assert(pb.target->size.width.type == Size::TYPE::PERCENTAGE);
					pb.target->size.width.set_meta(this->data.progress);
				}
				else
				{
					debug_assert(pb.target->size.height.type == Size::TYPE::PERCENTAGE);
					pb.target->size.height.set_meta(this->data.progress);
				}
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for progress bar.");
				debug_unreachable();
			}
		}

		template<typename T>
		void operator() (const T &)
		{
			debug_printline(0xffffffff, "GUI - update of unknown component type.");
		}
	};

	void update(engine::Asset window, engine::gui::Datas && datas)
	{
		// TODO: use thread safe queue

		for (auto & data : datas)
		{
			if (!lookup.contains(data.first))
			{
				debug_printline(0xffffffff, "GUI - cannot find named component for update.");
				continue;
			}

			engine::Entity entity = lookup.get<engine::Entity>(data.first);

			components.call(entity, Updater{ std::move(data.second) });
		}

		// updates and sends updated views to renderer (if shown)
		windows.get<Window>(window).update_window();
	}

	void update(engine::Asset window, core::maths::Vector3f delta)
	{
		// TODO: use thread safe queue
		windows.get<Window>(window).translate_window(delta);
	}
}
}
