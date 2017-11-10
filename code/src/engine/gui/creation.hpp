
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_CREATION_HPP
#define ENGINE_GUI_CREATION_HPP

#include "actions.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "update.hpp"
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

		using Components = core::container::UnorderedCollection
			<
			engine::Entity, 202,
			std::array<Function, 101>,
			std::array<View, 101>
			>;

		struct Creator
		{
			Actions & actions;
			Components & components;
			float depth = .0f;

			std::vector<std::pair<Asset, View*>> named_views;

			Creator(Actions & actions, Components & components)
				: actions(actions)
				, components(components)
			{}

			static Creator instantiate();

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
							std::move(viewData.function.templates[0])},
						view);

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

					debug_printline(0xffffffff, "GUI - invalid function type: ", viewData.function.type);
					debug_unreachable();
				}
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

				return view;
			}

			View & operator() (const TextureData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Texture>, data.texture },
					data);

				create_actions(view, data);
				create_function(view, data);

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
