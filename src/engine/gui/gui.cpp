
#include "actions.hpp"
#include "functions.hpp"
#include "gui.hpp"
#include "loading.hpp"
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
		engine::Entity, 301,
		// Views
		std::array<engine::gui::Group, 100>,
		std::array<engine::gui::List, 20>,
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

	struct MessageInteractionClick
	{
		engine::Entity entity;
	};
	struct MessageInteractionSelect
	{
		engine::Asset window;
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
		MessageInteractionSelect,
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

	struct Creator
	{
		Lookup & lookup;
		Window & window;

		Creator(Lookup & lookup, Window & window)
			: lookup(lookup)
			, window(window)
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

				lookup.put(data.function.name, entity);
				break;
			}
			default:
				break;
			}
		}

	public:

		View & operator() (const GroupData & data)
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

		View & operator() (const ListData & data)
		{
			const auto entity = engine::Entity::create();
			auto & view = components.emplace<List>(
				entity,
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
				data.gravity,
				data.margin,
				data.size,
				data.color,
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
				data.gravity,
				data.margin,
				data.size,
				data.color,
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
				data.gravity,
				data.margin,
				data.size,
				data.res,
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
		}
	}

	void destroy()
	{
		// TODO: destroy something
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
					}

					if (window.is_shown())
					{
						list.show(Vector3f{0.f, 0.f, 0.f});
					}
				}

				// hide item views if needed (if the update contains less items than previously)
				{
					const std::size_t items_remove = list.shown_items - list_size;

					for (std::size_t i = list.lookups.size() - items_remove; i < list.lookups.size(); i++)
					{
						list.children[i]->hide();
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

						components.call(lookup.get(kv.first), Updater{ window, std::move(kv.second) });
					}
				}

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

	void update()
	{
		struct
		{
			void operator () (MessageInteractionClick && x)
			{
				debug_assert(actions.contains(x.entity));
				actions.call(x.entity, Trigger{});
			}
			void operator () (MessageInteractionSelect && x)
			{
				select(x.window);
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

					components.call(lookup.get(data.first), Updater{ w, std::move(data.second) });
				}

				// updates and sends updated views to renderer (if shown)
				w.update_window();
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
	}

	void post_interaction_click(engine::Entity entity)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionClick>, entity);
		debug_assert(res);
	}

	void post_interaction_select(engine::Asset window)
	{
		const auto res = ::queue_posts.try_emplace(utility::in_place_type<MessageInteractionSelect>, window);
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
