
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
		extern Controllers controllers;
		extern Interactions interactions;
		extern Reactions reactions;
		extern Views views;
	}
}

namespace gameplay
{
	namespace gamestate
	{
		extern void post_gui(engine::Entity entity);
	}
}

namespace
{
	using namespace engine::gui;

	View & create(const float depth, View::Content && content, const ViewData & data, View *const parent)
	{
		const auto entity = engine::Entity::create();

		auto & view = views.emplace<View>(
			entity,
			entity,
			std::move(content),
			engine::Asset{ data.name },
			data.gravity,
			data.margin,
			data.size,
			parent);

		// a parent is always a group
		utility::get<View::Group>(parent->content).adopt(&view);

		view.depth = depth;
		return view;
	}

	class FindNode
	{
	public:
		const std::vector<ReactionData::Node> & observe;
		int index = 0;

		void * operator() (node_list_t & list)
		{
			// NOTE: is it possible to determine index here based on view parent? if we can find the list view parent...
			return is_finished() ? &list : visit(*this, list.nodes[observe[index++].index]);
		}
		void * operator() (node_map_t & node)
		{
			return is_finished() ? &node : visit(*this, find(node, observe[index++].key));
		}
		void * operator() (node_text_t & node)
		{
			if (!is_finished())
				throw bad_json{ "Reaction to 'text' is leaf node" };

			return &node;
		}

		node_t & find(node_map_t & node, engine::Asset key)
		{
			auto itr = node.nodes.find(key);
			if (itr == node.nodes.end())
			{
				// TODO: need better name feedback
				debug_printline("WARN - Cannot find node");
				throw key_missing{ "NA" };
			}
			return itr->second;
		}

		bool is_finished()
		{
			return index >= observe.size();
		}
	};

	auto & search_window(View & view)
	{
		View * parent = view.parent->parent;

		while (parent != nullptr)
		{
			auto p = parent->parent;

			if (p->parent == nullptr)
				return parent;

			parent = p;
		}

		return parent;
	}

	auto & search_parent(View & view, const std::string & parent_name)
	{
		auto name = engine::Asset{ parent_name };

		View * parent = view.parent;

		while (parent != nullptr)
		{
			if (parent->name == name)
				return *parent;

			parent = parent->parent;
		}

		throw bad_json{ "Could not find view named: ", parent_name };
	}

	void create_controller(View & view, const ViewData & data)
	{
		if (!data.has_controller())
			return;

		class Lookup
		{
		public:
			View & view;
			const ViewData & data;

			void operator() (const ControllerData::List & list_data)
			{
				auto & controller = controllers.emplace<ListController>(
					view.entity,
					list_data.item_template[0],
					view,
					utility::get<View::Group>(view.content));

				auto * node = static_cast<node_list_t*>(
					FindNode{ data.controller.reaction.observe }(reactions));

				// create reaction for the list
				node->reactions.emplace_back(&controller);
			}
			void operator() (const ControllerData::Tab & data)
			{

			}
			void operator() (const std::nullptr_t & data) { debug_unreachable(); }
		};

		visit(Lookup{ view, data }, data.controller.data);
	}

	void create_interaction(View & view, const ViewData & data)
	{
		for (auto & interaction : data.interactions)
		{
			view.selectable = true;

			engine::Entity target;

			switch (interaction.type)
			{
			case interaction_data_t::CLOSE:

				if (interaction.has_target())
				{
					target = search_parent(view, interaction.target).entity;
				}
				else
				{
					target = search_window(view)->entity;
				}
				interactions.emplace<action::close_t>(view.entity, target);
				break;

			case interaction_data_t::INTERACTION:

				target = interaction.has_target() ? search_parent(view, interaction.target).entity : view.entity;
				interactions.emplace<action::interaction_t>(view.entity, target);
				break;
			}
		}

		gameplay::gamestate::post_gui(view.entity);
	}

	void create_reaction(View & view, const ViewData & data)
	{
		if (!data.has_reaction())
			return;

		class Lookup
		{
		public:
			View & view;
			const ViewData & data;

			void operator() (const View::Color & content)
			{
				debug_printline("WARN - Reaction not supported for 'group'");
			}
			void operator() (const View::Group & content)
			{
				debug_printline("WARN - Reaction not supported for 'group'");
			}
			void operator() (const View::Text & content)
			{
				auto * node = static_cast<node_text_t*>(
					FindNode{ data.reaction.observe }(reactions));

				node->reactions.push_back(reaction_text_t{ &view });
			}
			void operator() (const View::Texture & content)
			{
				debug_printline("WARN - Reaction not supported for 'texture'");
			}
		};

		visit(Lookup{ view, data }, view.content);
	}

	class DataLookup
	{
	public:
		const Resources & resources;

		float depth;

		View * parent;

	public:

		DataLookup(const Resources & resources, const float depth)
			: resources(resources)
			, depth(depth)
		{
		}

	public:

		View & operator() (const GroupData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Group>, data.layout },
				data,
				parent);

			auto & content = utility::get<View::Group>(view.content);

			create_views(view, content, data);

			create_controller(view, data);
			create_interaction(view, data);
			create_reaction(view, data);

			//ViewUpdater::update(view, content);

			return view;
		}

		View & operator() (const PanelData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Color>, resource::color(data.color) },
				data,
				parent);

			create_controller(view, data);
			create_interaction(view, data);
			create_reaction(view, data);

			return view;
		}

		View & operator() (const TextData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Text>, data.display, resource::color(data.color) },
				data,
				parent);

			// update size base on initial string (if any)
			ViewUpdater::update(view, utility::get<View::Text>(view.content));

			create_controller(view, data);
			create_interaction(view, data);
			create_reaction(view, data);

			return view;
		}

		View & operator() (const TextureData & data)
		{
			View & view = create(
				depth,
				View::Content{ utility::in_place_type<View::Texture>, data.texture },
				data,
				parent);

			create_controller(view, data);
			create_interaction(view, data);
			create_reaction(view, data);

			return view;
		}

		View & create_view(View & parent, View::Group & parent_content, const DataVariant & data)
		{
			this->parent = &parent;

			View & view = visit(*this, data);

			depth += .01f;

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
		void create(const Resources & resources, View & screen_view, View::Group & screen_group, const DataVariant & window_data)
		{
			DataLookup data_lookup{ resources, screen_view.depth };

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
		void create(const Resources & resources, View & screen_view, View::Group & screen_group, std::vector<DataVariant> && windows_data)
		{
			for (auto & window_data : windows_data)
			{
				create(resources, screen_view, screen_group, window_data);
			}
		}
	}
}
