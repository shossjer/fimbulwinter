
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_CREATION_HPP
#define ENGINE_GUI_CREATION_HPP

#include "actions.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "noodle.hpp"
#include "update.hpp"
#include "react.hpp"
#include "view.hpp"

#include <core/container/Collection.hpp>

#include <vector>

namespace gameplay
{
	namespace gamestate
	{
		void post_gui(engine::Entity entity);
	}
}

namespace engine
{
	namespace gui
	{
		using Actions = core::container::MultiCollection
		<
			engine::Entity, 101,
			std::array<CloseAction, 20>,
			std::array<InteractionAction, 20>,
			std::array<TriggerAction, 20>
		>;

		using Components = core::container::Collection
		<
			engine::Entity, 401,
			std::array<Function, 101>,
			std::array<View, 101>
		>;

		struct Creator
		{
			Actions & actions;
			Components & components;
			data::Nodes & nodes;
			float depth;

			std::vector<std::pair<Asset, View*>> named_views;

			Creator(Actions & actions, Components & components, data::Nodes & nodes, float depth)
				: actions(actions)
				, components(components)
				, nodes(nodes)
				, depth(depth)
			{}

			// Creates and returns instance of Creator
			static Creator instantiate(float depth);

		public:

			template<typename T>
			static constexpr T & get_content(View & view)
			{
				return utility::get<T>(view.content);
			}

		private:

			View * find_view(const Asset name)
			{
				for (auto i = this->named_views.rbegin(); i != this->named_views.rend(); ++i)
				{
					if ((*i).first == name)
						return (*i).second;
				}
				return nullptr;
			}

			data::Node * find_node(const Asset key, data::Nodes & keyVals)
			{
				for (auto & keyVal : keyVals)
				{
					if (keyVal.first == key)
						return &(keyVal.second);
				}
				return nullptr;
			}

			data::Node & find_node(const std::vector<ViewData::React> & reaction)
			{
				data::Node * node = nullptr;
				data::Nodes * nodes_next = &this->nodes;

				for(auto & rnode : reaction)
				{
					node = find_node(rnode.key, *nodes_next);

					if (node == nullptr)	// TODO: better debug please
					{
						debug_printline("WARNING - cannot find key: ", rnode.key);
						throw key_missing{ "" };
					}

					nodes_next = &node->nodes;
				}

				return *node;
			}

			View & create(View::Content && content, const ViewData & data)
			{
				const auto entity = engine::Entity::create();

				auto & view = components.emplace<View>(
						entity,
						entity,
						std::move(content),
						data.gravity,
						data.margin,
						data.size,
						nullptr );

				if (!data.name.empty())
				{
					named_views.emplace_back(Asset{ data.name }, &view);
				}

				view.depth = this->depth;
				this->depth += .01f;
				return view;
			}

			Function & create(Function::Content && content, View & view)
			{
				const auto entity = engine::Entity::create();

				auto & function = components.emplace<Function>(
					entity,
					entity,
					std::move(content),
					&view);

				view.function = &function;

				return function;
			}

			void create_views(View & parent, View::Group & content, const GroupData & dataGroup)
			{
				for (auto & data : dataGroup.children)
				{
					View & view = visit(*this, data);

					// NOTE: can it be set during construction?
					view.parent = &parent;
					content.adopt(&view);
				}
			}

			void create_actions(View & view, const ViewData & viewData)
			{
				if (!viewData.actions.empty())
				{
					for (auto & data : viewData.actions)
					{
						Entity target = view.entity;

						if (data.target != Entity::null())
						{
							View * view = find_view(data.target);
							if (view != nullptr) target = view->entity;
							else
							{
								debug_printline(engine::gui_channel, "Could not find 'target' for action: ", data.target);
								continue;
							}
						}

						switch (data.type)
						{
						case ViewData::Action::CLOSE:
							this->actions.emplace<CloseAction>(view.entity, target);
							view.selectable = true;
							break;

						case ViewData::Action::INTERACTION:
							this->actions.emplace<InteractionAction>(view.entity, target);
							view.selectable = true;
							break;

						case ViewData::Action::TRIGGER:
							this->actions.emplace<TriggerAction>(view.entity, target);
							view.selectable = true;
							break;
						}
					}
					gameplay::gamestate::post_gui(view.entity);
				}
			}

			void create_function(View & view, const ViewData & viewData)
			{
				switch (viewData.function.type)
				{
				case Asset::null():
					break;
				case ViewData::Function::LIST:
				{
					auto & fun = create(
						Function::Content{
							utility::in_place_type<Function::List>,
							viewData.function.templates[0]},
						view);

					create_reaction(fun.entity, viewData.function.reaction);
					break;
				}
				case ViewData::Function::PROGRESS:
				{
					// TODO: progress

					break;
				}
				case ViewData::Function::TAB:
				{
					auto & fun = create(
						Function::Content{ utility::in_place_type<Function::TabCantroller> },
						view);

					auto tabs_group = find_view(Asset{ "tabs" });
					auto pages_group = find_view(Asset{ "views" });
					debug_assert(tabs_group != nullptr);
					debug_assert(pages_group != nullptr);

					auto & tabs_group_content = utility::get<View::Group>(tabs_group->content);
					auto & pages_group_content = utility::get<View::Group>(pages_group->content);
					debug_assert(tabs_group_content.children.size() == pages_group_content.children.size());
					debug_assert(!tabs_group_content.children.empty());

					auto & content = utility::get<Function::TabCantroller>(fun.content);
					content.tab_groups = tabs_group_content.children;
					content.page_groups = pages_group_content.children;

					for (std::size_t i = 1; i < content.page_groups.size(); i++)
					{
						ViewUpdater::hide(*content.page_groups[i]);
					}
					ViewUpdater::status(*content.tab_groups[0], Status::PRESSED);
					break;
				}
				default:

					debug_printline(engine::gui_channel, "GUI - invalid function type: ", viewData.function.type);
					debug_unreachable();
				}
			}

			void create_reaction(Entity entity, const std::vector<ViewData::React> & node_list)
			{
				// find the node..
				data::Node & node = find_node(node_list);

				// TODO: create actual reaction object

				// add reaction (entity) to node
				node.targets.emplace_back(entity);
			}

			void create_reaction(View & view, const ViewData & viewData)
			{
				if (!viewData.has_reaction())
					return;

				create_reaction(view.entity, viewData.reaction);
			}

		public:

			View & operator() (const GroupData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Group>, data.layout },
					data);

				auto & content = get_content<View::Group>(view);

				create_views(view, content, data);

				create_actions(view, data);
				create_function(view, data);
				create_reaction(view, data);

				ViewUpdater::update(view, content);

				return view;
			}

			View & operator() (const PanelData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Color>, resource::color(data.color) },
					data);

				create_actions(view, data);
				create_function(view, data);
				create_reaction(view, data);

				return view;
			}

			View & operator() (const TextData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Text>, data.display, resource::color(data.color) },
					data);

				ViewUpdater::update(view, get_content<View::Text>(view));

				create_actions(view, data);
				create_function(view, data);
				create_reaction(view, data);

				return view;
			}

			View & operator() (const TextureData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Texture>, data.texture },
					data);

				create_actions(view, data);
				create_function(view, data);
				create_reaction(view, data);

				return view;
			}

			View & create(View * parent, View::Group * parent_content, const DataVariant & data)
			{
				View & view = visit(*this, data);

				parent_content->adopt(&view);
				view.parent = parent;

				return view;
			}
		};
	}
}

#endif // ENGINE_GUI_CREATION_HPP
