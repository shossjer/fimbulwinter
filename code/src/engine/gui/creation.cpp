
#include "common.hpp"
#include "exception.hpp"
#include "loading.hpp"
#include "resources.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/Collection.hpp>

#include <vector>

namespace engine
{
	namespace gui
	{
		// define gui containters just for easy access
		extern Interactions interactions;
		extern Reactions reactions;
		extern Views views;
	}
}

namespace
{
	using namespace engine::gui;

	// TODO: move to some utility file
	template<typename T>
	constexpr T & get_content(View & view)
	{
		return utility::get<T>(view.content);
	}

	//View * find_view(const engine::Asset name)
	//{
	//	//for (auto i = this->named_views.rbegin(); i != this->named_views.rend(); ++i)
	//	//{
	//	//	if ((*i).first == name)
	//	//		return (*i).second;
	//	//}
	//	return nullptr;
	//}

	View & create(const float depth, View::Content && content, const ViewData & data)
	{
		const auto entity = engine::Entity::create();

		auto & view = views.emplace<View>(
			entity,
			entity,
			std::move(content),
			data.gravity,
			data.margin,
			data.size,
			nullptr);

		//if (!data.name.empty())
		//{
		//	named_views.emplace_back(Asset{ data.name }, &view);
		//}

		view.depth = depth;
		return view;
	}

	// interactions

	// reactions

	class DataLookup
	{
		const Resources & resources;

		float depth = 0.f;

	public:

		DataLookup(const Resources & resources)
			: resources(resources)
		{
		}

	private:

		void create_reaction(View & view, const ViewData & data)
		{
			if (!data.has_reaction())
				return;

			class
			{
			public:

				View & view;
				const std::vector<ReactionData::Node> & observe;
				int index = 0;

				void operator() (node_list_t & list)
				{
					if (is_finished())
					{
						list.reactions.emplace_back();
					}
					else
					{
						visit(*this, list.nodes[observe[index++].index]);
					}
				}
				void operator() (node_map_t & node)
				{
					if (is_finished())
					{
						debug_printline("WARN - Node map listener not implemented.");
					}
					else
					{
						visit(*this, find(node, observe[index++].key));
					}
				}
				void operator() (node_text_t & node)
				{
					if (is_finished())
					{
						node.reactions.push_back(reaction_text_t{ &view });
					}
					else
					{
						debug_printline("WARN - Text must be final node");
					}
				}

				node_t & find(node_map_t & node, engine::Asset key)
				{
					auto itr = node.nodes.find(key);
					if (itr == node.nodes.end())
					{
						// TODO: need better name feedback
						debug_printline("WARN - Cannot find node");
						throw key_missing{"NA"};
					}
					return itr->second;
				}

				bool is_finished()
				{
					return index >= observe.size();
				}
			} lookup{ view, data.reaction.observe };

			lookup(reactions);
		}

	public:

		View & operator() (const GroupData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Group>, data.layout },
				data);

			auto & content = get_content<View::Group>(view);

			create_views(view, content, data);

			//	create_actions(view, data);
			//	create_function(view, data);
			create_reaction(view, data);

			//ViewUpdater::update(view, content);

			return view;
		}

		View & operator() (const PanelData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Color>, resource::color(data.color) },
				data);

			//	create_actions(view, data);
			//	create_function(view, data);
			create_reaction(view, data);

			return view;
		}

		View & operator() (const TextData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Text>, data.display, resource::color(data.color) },
				data);

			// update size base on initial string (if any)
			ViewUpdater::update(view, get_content<View::Text>(view));

			//	create_actions(view, data);
			//	create_function(view, data);
			create_reaction(view, data);

			return view;
		}

		View & operator() (const TextureData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Texture>, data.texture },
				data);

			//	create_actions(view, data);
			//	create_function(view, data);
			create_reaction(view, data);

			return view;
		}

		View & create_view(View & parent, View::Group & parent_content, const DataVariant & data)
		{
			View & view = visit(*this, data);

			depth += .01f;

			// NOTE: can it be set during construction?
			parent_content.adopt(&view);
			view.parent = &parent;

			return view;
		}

	private:

		void create_views(View & parent, View::Group & content, const GroupData & dataGroup)
		{
			for (auto & data : dataGroup.children)
			{
				create_view(parent, content, data);
			}
		}
	};
}

namespace engine
{
	namespace gui
	{
		void create(const Resources & resources, View & screen_view, View::Group & screen_group, std::vector<DataVariant> && windows_data)
		{
			for (auto & window_data : windows_data)
			{
				DataLookup data_lookup{ resources };

				try
				{
					auto & window = data_lookup.create_view(screen_view, screen_group, window_data);
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
	}
}
