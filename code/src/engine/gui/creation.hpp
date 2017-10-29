
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_CREATION_HPP
#define ENGINE_GUI_CREATION_HPP

#include "actions.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/Collection.hpp>

namespace engine
{
	namespace gui
	{
		using Components = core::container::UnorderedCollection
			<
			engine::Entity, 202,
			std::array<Function, 101>,
			std::array<View, 101>
			>;

		struct Creator
		{
			Components & components;
			float depth = .0f;

			Creator(Components & components)
				: components(components)
			{}

		public:

			template<typename T>
			static constexpr T & get_content(View & view)
			{
				return utility::get<T>(view.content);
			}

		private:

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

				//	if (data.has_name()) lookup.put(data.name, entity);

				return view;
			}

			View & operator() (const PanelData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Color>, resource::color(data.color) },
					data);

				// not applicable for Color!
				//ViewUpdater::update(view, get_content<View::Color>(view));

				//create_action(view, data);
				//create_function(view, data);

				//if (data.has_name()) lookup.put(data.name, entity);

				return view;
			}

			View & operator() (const TextData & data)
			{
				View & view = create(
					View::Content{ utility::in_place_type<View::Text>, data.display, resource::color(data.color) },
					data);

				ViewUpdater::update(view, get_content<View::Text>(view));

				//create_action(view, data);
				//create_function(view, data);

				//if (data.has_name()) lookup.put(data.name, entity);

				return view;
			}

			View & operator() (const TextureData & data)
			{
				// TODO: Texture!
				View & view = create(
					View::Content{ utility::in_place_type<View::Color>, nullptr },
					data);

				// not applicable for Color!
				//ViewUpdater::update(view, get_content<View::Color>(view));

				//create_action(view, data);

				//if (data.has_name()) lookup.put(data.name, entity);

				return view;
			}
		};
	}
}

#endif // ENGINE_GUI_CREATION_HPP
