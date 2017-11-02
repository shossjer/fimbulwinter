
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
		void post_add(engine::Entity entity);
	}
}

namespace engine
{
	namespace gui
	{
		using Actions = core::container::MultiCollection
			<
			engine::Entity, 101,
			std::array<InteractionAction, 20>
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
						case ViewData::Action::INTERACTION:

							this->actions.emplace<InteractionAction>(view.entity, target);
							view.selectable = true;
							break;
						}
					}
					gameplay::gamestate::post_add(view.entity);
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

				ViewUpdater::update(view, content);

				//if (data.has_name()) lookup.put(data.name, entity);

				//if (data.function.type != engine::Asset::null())
				//{
				//	const auto function_entity = engine::Entity::create();

				//	if (!data.function.name.empty())
				//		lookup.put(data.function.name, function_entity);

				//	create_views(view, data);
				//	create_function(function_entity, view, data);
				//}
				//else
				//{
				//	create_views(view, data);
				//}

				return view;
			}

			View & operator() (const ListData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Group>, data.layout },
					data);

				ViewUpdater::update(view, get_content<View::Group>(view));

				// TODO: List!

				return view;
			}

			View & operator() (const PanelData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Color>, resource::color(data.color) },
					data);

				create_actions(view, data);
				//create_function(view, data);

				return view;
			}

			View & operator() (const TextData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Text>, data.display, resource::color(data.color) },
					data);

				ViewUpdater::update(view, get_content<View::Text>(view));

				create_actions(view, data);
				//create_function(view, data);

				return view;
			}

			View & operator() (const TextureData & data)
			{
				// TODO: Texture!
				View & view = create(
					View::Content{ utility::in_place_type<View::Color>, nullptr },
					data);

				//create_action(view, data);

				return view;
			}
		};
	}
}

#endif // ENGINE_GUI_CREATION_HPP
