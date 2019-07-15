
#include "gui.hpp"

#include "common.hpp"
#include "loading.hpp"
#include "reaction.hpp"
#include "view_refresh.hpp"

#include <engine/console.hpp>

#include <core/container/CircleQueue.hpp>

namespace engine
{
	namespace gui
	{
		Controllers controllers;

		Interactions interactions;

		Reactions reactions;

		Resources resources;

		Views views;

		extern void create(const Resources & resources, View & screen_view, View::Group & screen_group, std::vector<DataVariant> && windows_data);

		extern void clear(Reactions & reactions);

		extern void setup(MessageDataSetup & data, Reactions & reactions);

		extern void update(const Resources & resources, MessageData & data, Reactions & reactions, Views & views);

		extern void update(const Resources & resources, MessageInteraction & data);
	//	extern void update(const Resources & resources, MessageInteraction & data, Interactions & interactions, Views & views);
	}
}

using namespace engine::gui;

namespace
{
	const uint32_t WINDOW_HEIGHT = 677; // 720
	const uint32_t WINDOW_WIDTH = 1004; // 1024

	View * screen_view = nullptr;

	using UpdateMessage = utility::variant
	<
		MessageData,
		MessageDataSetup,
		MessageInteraction,
		MessageReload,
		MessageVisibility
	>;

	core::container::CircleQueueSRMW<UpdateMessage, 100> queue_posts;

	void clear()
	{
		controllers.clear();

		//TODO: interactions.clear();

		clear(reactions);

		for (View & view : ::views.get<View>())
		{
			ViewRenderer::remove(view);
		}

		views.clear();

		resource::purge();
	}

	View::Group & reset()
	{
		clear();

		// create the "screen" view of the screen
		// TODO: use "window size" as size param
		const auto entity = engine::Entity::create();
		screen_view = &views.emplace<View>(
			entity,
			entity,
			View::Group{ Layout::RELATIVE },
			engine::Asset{"screen"},
			Gravity{},
			Margin{},
			Size{ { Size::FIXED, height_t{ WINDOW_HEIGHT }, },{ Size::FIXED, width_t{ WINDOW_WIDTH } } },
			nullptr);

		return utility::get<View::Group>(screen_view->content);
	}

	struct MessageLookup
	{
		void operator() (MessageData && m)
		{
			// inform 'reaction' about new data available
			engine::gui::update(resources, m, reactions, views);
		}
		void operator() (MessageDataSetup && m)
		{
			// setup the 'reaction' structure
			setup(m, reactions);
		}
		void operator() (MessageInteraction && m)
		{
			if (interactions.contains(m.entity))
			{
				// manage player interaction
				update(resources, m);
			}
		}
		void operator() (MessageReload && m)
		{
			debug_printline(engine::gui_channel, "Reloading GUI");

			View::Group & screen_group = reset();

			create(resources, *screen_view, screen_group, load());
		}
		void operator() (MessageVisibility && m)
		{
			// TODO: window visibility update
		}
	};
}

namespace engine
{
	namespace gui
	{
		void create()
		{
			// register callback with console input
			engine::console::observe("gui-reload", std::function<void()>
			{
				[&]()
				{
					const auto res = queue_posts.try_emplace(utility::in_place_type<MessageReload>, MessageReload{});
					debug_assert(res);
				}
			});
		}

		void destroy()
		{
			::clear();
		}

		void update()
		{
			UpdateMessage message;
			while (::queue_posts.try_pop(message))
			{
				visit(MessageLookup{}, std::move(message));
			}

			// allow view's to update based on reaction or interaction changes (if any)
			ViewRefresh::refresh(*screen_view);
		}

		template<typename T>
		void put_on_queue(T && data)
		{
			const auto res = queue_posts.try_emplace(utility::in_place_type<T>, std::move(data));
			debug_assert(res);
		}

		template<> void post(MessageData && data) { put_on_queue(std::move(data)); }
		template<> void post(MessageDataSetup && data) { put_on_queue(std::move(data)); }
		template<> void post(MessageInteraction && data) { put_on_queue(std::move(data)); }
		template<> void post(MessageReload && data) { put_on_queue(std::move(data)); }
		template<> void post(MessageVisibility && data) { put_on_queue(std::move(data)); }
	}
}
