
#include "creation.hpp"
#include "gui.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "measure.hpp"
#include "noodle.hpp"
#include "update.hpp"
#include "react.hpp"
#include "view.hpp"

#include <engine/console.hpp>

#include <core/container/CircleQueue.hpp>

#include <vector>

using namespace engine::gui;

constexpr engine::Asset ViewData::Action::CLOSE;
constexpr engine::Asset ViewData::Action::INTERACTION;
constexpr engine::Asset ViewData::Action::MOVER;
constexpr engine::Asset ViewData::Action::SELECT;
constexpr engine::Asset ViewData::Action::TRIGGER;

constexpr engine::Asset ViewData::Function::LIST;
constexpr engine::Asset ViewData::Function::PROGRESS;
constexpr engine::Asset ViewData::Function::TAB;

namespace
{
	const uint32_t WINDOW_HEIGHT = 677; // 720
	const uint32_t WINDOW_WIDTH = 1004; // 1024

	Actions actions;

	Components components;

	data::Nodes nodes;

	View * screen_view = nullptr;
	View::Group * screen_group = nullptr;

	// TODO: update lookup structure
	// init from "gameplay" data structures
	// assign views during creation
	core::container::Collection<engine::Asset, 11, std::array<View*, 10>> lookup;

	using UpdateMessage = utility::variant
	<
		MessageData,
		MessageDataSetup,
		MessageInteraction,
		MessageReload,
		MessageVisibility
	>;

	core::container::CircleQueueSRMW<UpdateMessage, 100> queue_posts;
}

namespace engine
{
	namespace gui
	{
		extern std::vector<DataVariant> load();

		Creator Creator::instantiate(float depth) { return Creator{ ::actions, ::components, ::nodes, depth }; }

		void create()
		{
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
			// TODO: destroy something
		}

		void update()
		{
			struct
			{
				static data::Node & find(const Asset key, data::Nodes & nodes)
				{
					for (auto & node : nodes)
						if (node.first == key)
							return node.second;
					debug_unreachable();
				}
				void operator() (MessageData && x)
				{
					struct ParseUpdate
					{
						data::Node & node;

						void operator() (const data::Values & values)
						{
							debug_assert(!node.nodes.empty());

							// First; check if new Nodes needs to be added
							for (std::size_t i = node.nodes.size(); i < values.data.size(); i++)
							{
								// Note: calls Setup
								visit(data::ParseSetup{
									Asset{ std::to_string(i) },
									node.nodes },
									values.data[i]);
							}

							// Second; update all Lists so they can adjust number of items
							ListReaction reaction{ values };

							for (auto target : node.targets)
							{
								::components.call(target, reaction);
							}

							// Third; update List-Items with new data
							for (std::size_t i = 0; i < values.data.size(); i++)
							{
								auto & value = values.data[i];
								auto & node = this->node.nodes[i];

								visit(ParseUpdate{ node.second }, value);
							}
						}
						void operator() (const data::KeyValues & map)
						{
							for (auto & data : map.data)
							{
								visit(ParseUpdate{ find(data.first, node.nodes) }, data.second);
							}
						}
						void operator() (const std::nullptr_t)
						{
							for (auto target : node.targets)
							{
								// TODO: inform targets of update
							}
						}
						void operator() (const std::string & data)
						{
							TextReaction reaction{ data };

							for (auto target : node.targets)
							{
								::components.call(target, reaction);
							}
						}
					};
					visit(ParseUpdate{ find(x.data.first, ::nodes) }, x.data.second);
				}
				void operator () (MessageDataSetup && x)
				{
					visit(data::ParseSetup{ x.data.first, ::nodes }, x.data.second);
				}

				void operator () (MessageInteraction && x)
				{
					debug_printline(engine::gui_channel, "MessageInteraction");

					struct Updater
					{
						MessageInteraction & message;

						void operator() (const CloseAction & action)
						{
							if (message.interaction == MessageInteraction::CLICK)
							{
								ViewUpdater::hide(components.get<View>(action.target));
							}
						}
						void operator() (const InteractionAction & action)
						{
							Status::State state;
							switch (message.interaction)
							{
							case MessageInteraction::HIGHLIGHT:
								state = Status::HIGHLIGHT;
								break;
							case MessageInteraction::LOWLIGHT:
								state = Status::DEFAULT;
								break;
							case MessageInteraction::PRESS:
								state = Status::PRESSED;
								break;
							case MessageInteraction::RELEASE:
								state = Status::HIGHLIGHT;
								break;
							default:
								return;
							}

							ViewUpdater::status(
								components.get<View>(action.target),
								state);
						}
						void operator() (const TriggerAction & action)
						{
							if (message.interaction == MessageInteraction::CLICK)
							{
								View & view = components.get<View>(action.target);

								visit(fun::Trigger{ message.entity, view.function }, view.function->content);
							}
						}
					};
					actions.call_all(x.entity, Updater{ x });
				}
				void operator () (MessageReload && x)
				{
					debug_printline(engine::gui_channel, "Reloading GUI");
					// Clear all prev. data
					{
						::actions = Actions{};

						for (View & view : ::components.get<View>())
						{
							ViewRenderer::remove(view);
						}

						::components.clear();

						::lookup.clear();

						resource::purge();
					}

					// create the "screen" view of the screen
					// TODO: use "window size" as size param
					const auto entity = engine::Entity::create();
					screen_view = &components.emplace<View>(
						entity,
						entity,
						View::Group{ View::Group::Layout::RELATIVE },
						Gravity{},
						Margin{},
						Size{ { Size::FIXED, height_t{ WINDOW_HEIGHT }, },{ Size::FIXED, width_t{ WINDOW_WIDTH } } },
						nullptr);

					screen_group = &utility::get<View::Group>(screen_view->content);

					// (Re)load data windows and views data from somewhere.
					std::vector<DataVariant> windows_data;// = load();

					for (auto & window_data : windows_data)
					{
						try
						{
							auto & view = Creator::instantiate(0.f).create(screen_view, screen_group, window_data);

							// temp
							const GroupData & data = utility::get<GroupData>(window_data);
							lookup.emplace<View*>(Asset{ data.name }, &view);
						}
						catch (key_missing & e)
						{
							debug_printline(engine::gui_channel,
								"Exception - Could not find data-mapping: ", e.message);
						}
						catch (bad_json & e)
						{
							debug_printline(engine::gui_channel,
								"Exception - Something not right in JSON: ", e.message);
						}
						catch (exception & e)
						{
							debug_printline(engine::gui_channel,
								"Exception creating window: ", e.message);
						}
					}
				}
				void operator () (MessageVisibility && x)
				{
					debug_printline(engine::gui_channel, "MessageVisibility");

					if (!lookup.contains(x.window))
						return;

					View & view = *lookup.get<View*>(x.window);

					switch (x.state)
					{
					case MessageVisibility::HIDE:
						ViewUpdater::hide(view);
						break;
					case MessageVisibility::SHOW:
						ViewUpdater::show(view);
						break;
					case MessageVisibility::TOGGLE:
						if (view.status.should_render)
							ViewUpdater::hide(view);
						else
							ViewUpdater::show(view);
						break;
					}
				}
			} processMessage;

			UpdateMessage message;
			while (::queue_posts.try_pop(message))
			{
				visit(processMessage, std::move(message));
			}

			ViewMeasure::refresh(*screen_view);
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
