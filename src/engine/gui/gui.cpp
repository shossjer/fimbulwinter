
#include "actions.hpp"
#include "functions.hpp"
#include "gui.hpp"
#include "loading.hpp"
#include "resources.hpp"
#include "views.hpp"

#include <core/container/CircleQueue.hpp>
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

	core::container::MultiCollection
	<
		engine::Entity, 101,
		std::array<CloseAction, 10>,
		std::array<InteractionAction, 10>,
		std::array<MoveAction, 10>,
		std::array<SelectAction, 10>,
		std::array<TriggerAction, 10>
	>
	actions;

	core::container::Collection
	<
		engine::Entity, 301,
		// Views
		std::array<engine::gui::Group, 100>,
		std::array<engine::gui::List, 20>,
		std::array<engine::gui::PanelC, 50>,
		std::array<engine::gui::PanelT, 50>,
		std::array<engine::gui::Text, 50>,
		// Functionality
		std::array<engine::gui::ProgressBar, 20>,
		std::array<engine::gui::TabBar, 5>
	>
	components;

	core::container::Collection
	<
		engine::Asset, 21,
		std::array<engine::gui::Window, 10>,
		std::array<int, 10>
	>
	windows;

	struct MessageInteractionClick
	{
		engine::Asset window;
		engine::Entity entity;
	};
	struct MessageInteractionHighlight
	{
		engine::Asset window;
		engine::Entity entity;
	};
	struct MessageInteractionLowlight
	{
		engine::Asset window;
		engine::Entity entity;
	};
	struct MessageInteractionPress
	{
		engine::Asset window;
		engine::Entity entity;
	};
	struct MessageInteractionRelease
	{
		engine::Asset window;
		engine::Entity entity;
	};

	struct MessageStateHide
	{
		engine::Asset window;
	};
	struct MessageStateShow
	{
		engine::Asset window;
	};
	struct MessageStateToggle
	{
		engine::Asset window;
	};
	struct MessageUpdateData
	{
		engine::Asset window;
		Datas datas;
	};
	struct MessageUpdateTranslate
	{
		engine::Asset window;
		core::maths::Vector3f delta;
	};

	using UpdateMessage = utility::variant
	<
		MessageInteractionClick,
		MessageInteractionHighlight,
		MessageInteractionLowlight,
		MessageInteractionPress,
		MessageInteractionRelease,
		MessageStateHide,
		MessageStateShow,
		MessageStateToggle,
		MessageUpdateData,
		MessageUpdateTranslate
	>;

	core::container::CircleQueueSRMW<UpdateMessage, 100> queue_posts;

	// lookup table for "named" components (asset -> entity)
	// used when gamestate requests updates of components.
	Lookup lookup;

	std::vector<Window *> window_stack;

	engine::Entity find_entity(const engine::Asset name, const Lookup & lookup)
	{
		if (!lookup.contains(name))
		{
			debug_printline(0xffffffff, "GUI - could not find asset in lookup.");
			debug_unreachable();
		}

		return lookup.get(name);
	}
	View * find_view(const engine::Asset name, const Lookup & lookup)
	{
		if (!lookup.contains(name))
			return nullptr;

		auto entity = lookup.get(name);

		struct
		{
			View * operator() (Group & view) { return &view; }
			View * operator() (List & view) { return &view; }
			View * operator() (PanelC & view) { return &view; }
			View * operator() (PanelT & view) { return &view; }
			View * operator() (Text & view) { return &view; }

			View * operator() (const ProgressBar &) { return nullptr; }
			View * operator() (const TabBar &) { return nullptr; }
		}
		finder;

		return components.call(entity, finder);
	}

	struct Creator
	{
		Lookup & lookup;
		Window & window;

		Creator(Lookup & lookup, Window & window)
			: lookup(lookup)
			, window(window)
		{}

	private:

		void create_action_close(Drawable & drawable, const ViewData::Action & data)
		{
			View * view;

			if (data.target == engine::Asset::null())
				view = &drawable;
			else
			{
				view = find_view(data.target, this->lookup);
				debug_assert(view != nullptr);
			}

			actions.emplace<engine::gui::CloseAction>(drawable.entity, this->window.name(), view);
		}
		void create_action_interaction(Drawable & drawable, const ViewData::Action & data)
		{
			View * view;

			if (data.target == engine::Asset::null())
				view = &drawable;
			else
			{
				view = find_view(data.target, this->lookup);
				debug_assert(view != nullptr);
			}

			actions.emplace<engine::gui::InteractionAction>(drawable.entity, this->window.name(), view);
		}
		void create_action_mover(Drawable & drawable, const ViewData::Action & data)
		{

		}
		void create_action_select(Drawable & drawable, const ViewData::Action & data)
		{

		}
		void create_action_trigger(Drawable & drawable, const ViewData::Action & data)
		{
			engine::Entity entity;

			if (data.target == engine::Asset::null())
				entity = drawable.entity;
			else
				entity = find_entity(data.target, this->lookup);

			actions.emplace<engine::gui::TriggerAction>(drawable.entity, this->window.name(), entity);
		}

		void create_action(Drawable & drawable, const ViewData & datas)
		{
			for (auto & data : datas.actions)
			{
				switch (data.type)
				{
				case ViewData::Action::CLOSE:
					create_action_close(drawable, data);
					break;

				case ViewData::Action::INTERACTION:
					create_action_interaction(drawable, data);
					break;

				case ViewData::Action::MOVER:

					actions.emplace<engine::gui::MoveAction>(drawable.entity, engine::gui::MoveAction{ this->window.name() });
					break;

				case ViewData::Action::SELECT:

					actions.emplace<engine::gui::SelectAction>(drawable.entity, engine::gui::SelectAction{ this->window.name() });
					break;
				case ViewData::Action::TRIGGER:

					create_action_trigger(drawable, data);
					break;
				}

				gameplay::gamestate::post_add(drawable.entity, this->window.name(), data.type);
			}
		}

		void create_function(engine::Entity entity, View & view, const ViewData & data)
		{
			switch (data.function.type)
			{
			case ViewData::Function::PROGRESS:
			{
				components.emplace<ProgressBar>(
					entity,
					this->window.name(),
					&view,
					data.function.direction);

				break;
			}
			case ViewData::Function::TAB:
			{
				Group * const base = dynamic_cast<Group*>(&view);
				debug_assert(base != nullptr);

				auto & tabBar = components.emplace<TabBar>(
					entity,
					this->window.name(),
					base,
					dynamic_cast<Group*>(base->find("tabs")),
					dynamic_cast<Group*>(base->find("views")));

				debug_assert(tabBar.tab_container != nullptr);
				debug_assert(tabBar.views_container != nullptr);
				debug_assert(tabBar.tab_container->children.size() == tabBar.views_container->children.size());

				for (std::size_t i = 1; i < tabBar.views_container->children.size(); i++)
				{
					auto content = tabBar.views_container->children[i];
					content->visibility_hide();
				}
				tabBar.tab_container->children[0]->update(View::State::PRESSED);
				break;
			}
			default:

				debug_printline(0xffffffff, "GUI - invalid function type.");
				debug_unreachable();
			}
		}
		void create_function(View & view, const ViewData & data)
		{
			if (data.function.type == engine::Asset::null())
				return;

			const auto entity = engine::Entity::create();

			if (!data.function.name.empty())
				lookup.put(data.function.name, entity);

			create_function(entity, view, data);
		}

	public:

		View & operator() (const GroupData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<Group>(
				entity,
				entity,
				data.name,
				data.gravity,
				data.margin,
				data.size,
				data.layout);

			if (data.has_name()) lookup.put(data.name, entity);

			if (data.function.type != engine::Asset::null())
			{
				const auto function_entity = engine::Entity::create();

				if (!data.function.name.empty())
					lookup.put(data.function.name, function_entity);

				create_views(view, data);
				create_function(function_entity, view, data);
			}
			else
			{
				create_views(view, data);
			}

			return view;
		}

		View & operator() (const ListData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<List>(
				entity,
				entity,
				data.name,
				data.gravity,
				data.margin,
				data.size,
				data.layout,
				data);

			if (data.has_name()) lookup.put(data.name, entity);

			return view;
		}

		View & operator() (const PanelData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<PanelC>(
				entity,
				entity,
				data.name,
				data.gravity,
				data.margin,
				data.size,
				resource::color(data.color),
				data.has_action());
			create_action(view, data);
			create_function(view, data);

			if (data.has_name()) lookup.put(data.name, entity);

			return view;
		}

		View & operator() (const TextData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<Text>(
				entity,
				entity,
				data.name,
				data.gravity,
				data.margin,
				data.size,
				resource::color(data.color),
				data.display);
			create_action(view, data);
			create_function(view, data);

			if (data.has_name()) lookup.put(data.name, entity);

			return view;
		}

		View & operator() (const TextureData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<PanelT>(
				entity,
				entity,
				data.name,
				data.gravity,
				data.margin,
				data.size,
				data.texture,
				data.has_action());
			create_action(view, data);

			if (data.has_name()) lookup.put(data.name, entity);

			return view;
		}

		void create_views(Group & parent, const GroupData & dataGroup)
		{
			for (auto & data : dataGroup.children)
			{
				View & view = visit(*this, data);

				parent.adopt(&view);
			}
		}
	};

	void select(const engine::Asset window)
	{
		for (std::size_t i = 0; i < window_stack.size(); i++)
		{
			if (window_stack[i]->name() == window)
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

	void hide(const engine::Asset window)
	{
		windows.get<Window>(window).hide_window();
	}

	void show(const engine::Asset window)
	{
		select(window);
		windows.get<Window>(window).show_window();
	}
}

namespace engine
{
namespace gui
{
	extern void load(std::vector<DataVariant> & windows);

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

			Creator(lookup, window).create_views(window.group, data);

			window.init_window();
		}
	}

	void destroy()
	{
		// TODO: destroy something
	}

	struct Trigger
	{
		engine::Entity caller;

		void operator() (TabBar & function)
		{
			std::size_t clicked_index = function.active_index;
			for (std::size_t i = 0; i < function.tab_container->children.size(); i++)
			{
				if (function.tab_container->children[i]->find(this->caller) != nullptr)
				{
					clicked_index = i;
					break;
				}
			}

			if (function.active_index == clicked_index)
				return;

			//// hide / unselect the prev. tab
			function.tab_container->children[function.active_index]->update(View::State::DEFAULT);
			function.views_container->children[function.active_index]->visibility_hide();

			//// show / select the new tab.
			function.tab_container->children[clicked_index]->update(View::State::PRESSED);
			function.views_container->children[clicked_index]->visibility_show();
			function.views_container->children[clicked_index]->renderer_show();

			function.active_index = clicked_index;
		}

		template<typename T>
		void operator() (const T&) {}
	};

	struct InteractionClick
	{
		engine::Entity caller;

		void operator() (const CloseAction & action)
		{
			hide(action.window);
		}

		void operator() (const TriggerAction & action)
		{
			components.call(action.entity, Trigger{ caller });
		}

		template<typename T>
		void operator() (const T &) {}
	};

	struct
	{
		void operator() (const InteractionAction & action)
		{
			action.target->update(View::State::DEFAULT);
		}
		template<typename T>
		void operator() (const T &) {}
	}
	selectionStateDefault;

	struct
	{
		void operator() (const InteractionAction & action)
		{
			action.target->update(View::State::HIGHLIGHT);
		}
		template<typename T>
		void operator() (const T &) {}
	}
	selectionStateHighlight;

	struct
	{
		void operator() (const InteractionAction & action)
		{
			action.target->update(View::State::PRESSED);
		}
		template<typename T>
		void operator() (const T &) {}
	}
	selectionStatePressed;

	struct Updater
	{
		Window & window;
		Data & data;

		void operator() (List & list)
		{
			switch (this->data.type)
			{
			case Data::LIST:
			{
				const std::size_t list_size = this->data.list.size();

				// create item views if needed
				if (list.lookups.capacity() < list_size)
				{
					list.lookups.reserve(list_size);

					// create item views to match the size
					for (std::size_t i = list.lookups.size(); i < list_size; i++)
					{
						list.lookups.emplace_back();
						Creator{list.lookups[i], window}.create_views(list, list.view_template);
						list.children.back()->translate(list.order);

						if (window.is_shown())
						{
							list.children[i]->renderer_show();
						}
					}
				}

				// hide item views if needed (if the update contains less items than previously)
				{
					const std::size_t items_remove = list.shown_items - list_size;

					for (std::size_t i = list.lookups.size() - items_remove; i < list.lookups.size(); i++)
					{
						list.children[i]->renderer_hide();
					}
				}

				// update the view data
				for (std::size_t i = 0; i < list_size; i++)
				{
					auto & item = this->data.list[i];

					for (auto & kv : item)
					{
						auto & lookup = list.lookups[i];

						if (!lookup.contains(kv.first))
						{
							debug_printline(0xffffffff, "GUI - cannot find named component for list.");
							continue;
						}

						components.call(lookup.get(kv.first), Updater{ window, kv.second });
					}
				}

				list.set_update((list.shown_items != list_size) ? 2 : 1);
				list.shown_items = list_size;
				break;
			}
			default:

				debug_printline(0xffffffff, "GUI - invalid update type for list (needs list).");
				debug_unreachable();
			}
		}

		void operator() (PanelC & panel)
		{
			switch (this->data.type)
			{
			case Data::COLOR:

				//panel.color = data.color;
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
				panel.set_update(2);
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

				//text.color = data.color;
				break;

			case Data::DISPLAY:

				text.display = data.display;
				text.set_update((text.size.width == Size::TYPE::WRAP) ? 2 : 1);
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
					debug_assert(pb.target->size.width.type == Size::TYPE::PERCENT);
					pb.target->size.width.set_meta(this->data.progress);
				}
				else
				{
					debug_assert(pb.target->size.height.type == Size::TYPE::PERCENT);
					pb.target->size.height.set_meta(this->data.progress);
				}
				pb.target->set_update(2);
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

	void update()
	{
		struct
		{
			void operator () (MessageInteractionClick && x)
			{
				debug_assert(actions.contains(x.entity));
				actions.call_all(x.entity, InteractionClick{ x.entity });

				windows.get<Window>(x.window).splash_mud();
			}
			void operator () (MessageInteractionHighlight && x)
			{
				if (actions.contains(x.entity))
					actions.call_all(x.entity, selectionStateHighlight);

				windows.get<Window>(x.window).splash_mud();
			}
			void operator () (MessageInteractionLowlight && x)
			{
				if (actions.contains(x.entity))
					actions.call_all(x.entity, selectionStateDefault);

				windows.get<Window>(x.window).splash_mud();
			}
			void operator () (MessageInteractionPress && x)
			{
				// check if window should be selected
				if (window_stack.front()->name() != x.window)
				{
					select(x.window);
				}

				if (actions.contains(x.entity))
					actions.call_all(x.entity, selectionStatePressed);

				windows.get<Window>(x.window).splash_mud();
			}
			void operator () (MessageInteractionRelease && x)
			{
				if (actions.contains(x.entity))
					actions.call_all(x.entity, selectionStateHighlight);

				windows.get<Window>(x.window).splash_mud();
			}
			void operator () (MessageStateHide && x)
			{
				hide(x.window);
			}
			void operator () (MessageStateShow && x)
			{
				show(x.window);
			}
			void operator () (MessageStateToggle && x)
			{
				auto & w = windows.get<Window>(x.window);

				if (w.is_shown()) w.hide_window();
				else w.show_window();
			}
			void operator () (MessageUpdateData && x)
			{
				Window & w = windows.get<Window>(x.window);

				for (auto & data : x.datas)
				{
					if (!lookup.contains(data.first))
					{
						debug_printline(0xffffffff, "GUI - cannot find named component for update.");
						continue;
					}

					components.call(lookup.get(data.first), Updater{ w, data.second });
				}

				w.splash_mud();
			}
			void operator () (MessageUpdateTranslate && x)
			{
				windows.get<Window>(x.window).translate_window(x.delta);
			}

		} processMessage;

		UpdateMessage message;
		while (::queue_posts.try_pop(message))
		{
			visit(processMessage, std::move(message));
		}

		for (auto & window : windows.get<Window>())
		{
			if (window.is_dirty())
			{
				window.refresh_window();
			}
		}
	}

	void post_interaction_click(engine::Asset window, engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionClick>, window, entity);
		debug_assert(res);
	}

	void post_interaction_highlight(engine::Asset window, engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionHighlight>, window, entity);
		debug_assert(res);
	}

	void post_interaction_lowlight(engine::Asset window, engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionLowlight>, window, entity);
		debug_assert(res);
	}

	void post_interaction_press(engine::Asset window, engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionPress>, window, entity);
		debug_assert(res);
	}

	void post_interaction_release(engine::Asset window, engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionRelease>, window, entity);
		debug_assert(res);
	}

	void post_state_hide(engine::Asset window)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageStateHide>, window);
		debug_assert(res);
	}

	void post_state_show(engine::Asset window)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageStateShow>, window);
		debug_assert(res);
	}

	void post_state_toggle(engine::Asset window)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageStateToggle>, window);
		debug_assert(res);
	}

	void post_update_data(
		engine::Asset window,
		Datas && datas)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageUpdateData>, window, std::move(datas));
		debug_assert(res);
	}

	void post_update_translate(
		engine::Asset window,
		core::maths::Vector3f delta)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageUpdateTranslate>, window, std::move(delta));
		debug_assert(res);
	}
}
}
