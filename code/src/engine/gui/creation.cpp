
#include "common.hpp"
#include "controller.hpp"
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
			for (auto & pair : node.nodes)
			{
				if (pair.first == key)
					return pair.second;
			}

			debug_printline("WARN - Cannot find node");
			throw key_missing{ "NA" };
		}

		bool is_finished()
		{
			return index >= observe.size();
		}
	};

	View * search_window(View & view)
	{
		View * parent = view.parent->parent;

		while (parent != nullptr)
		{
			auto p = parent->parent;

			if (p->parent == nullptr)
				break;

			parent = p;
		}

		return parent;
	}

	View * find_child(View & parent, const std::string & name)
	{
		struct
		{
			engine::Asset key;

			View * operator() (View::Group & group)
			{
				for (auto child : group.children)
				{
					if (child->name == key)
						return child;

					View * view = visit(*this, child->content);

					if (view != nullptr)
						return view;
				}

				return nullptr;
			}

			View * operator() (const View::Color &) { return nullptr; }
			View * operator() (const View::Text &) { return nullptr; }
			View * operator() (const View::Texture &) { return nullptr; }
		}
		lookup{ engine::Asset{ name } };

		if (lookup.key == parent.name)
			return &parent;

		return visit(lookup, parent.content);
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

			void operator() (const controller_data_t::list_t & list_data)
			{
				auto & controller = controllers.emplace<controller::list_t>(
					view.entity,
					list_data.item_template[0],
					view,
					utility::get<View::Group>(view.content));

				auto * node = static_cast<node_list_t*>(
					FindNode{ data.controller.reaction.observe }(reactions));

				// create reaction for the list
				node->reactions.emplace_back(controller);
			}
			void operator() (const controller_data_t::tab_t & data)
			{
				View * pager = find_child(view, data.pager_name);

				if (pager == nullptr)
				{
					throw bad_json{ "Cannot find group view named: ", data.pager_name, " for tab controller." };
				}

				auto & controller = controllers.emplace<controller::tab_t>(
					view.entity,
					*pager);

				View * tabs_view = find_child(view, data.tabs_name);
				auto & tabs_group = utility::get<View::Group>(tabs_view->content);

				controller.tabs = tabs_group.children;

				for (View * tab_view : controller.tabs)
				{
					View * tab_clickable = find_child(*tab_view, "tab_trigger");

					if (tab_clickable == nullptr)
					{
						throw bad_json{ "Each tab layout needs to contain renderable view with name 'tab_trigger'" };
					}

					// create tab click action for each tab
					interactions.emplace<action::tab_t>(tab_clickable->entity, tab_view->entity, view.entity);
				}

				controller.active_tab = controller.tabs[0];

				auto & pager_group = utility::get<View::Group>(controller.pager_view.content);

				// assert number of tabs matches number of pages
				if (pager_group.children.size() != controller.tabs.size())
				{
					throw bad_json{ "Controller for 'tabs' must have same number of tabs as pages." };
				}
				if (pager_group.children.empty())
				{
					return;
				}

				// the controller needs a pointer to each available page
				// remove all but the first page from the actual pager
				controller.pages = std::move(pager_group.children);
				pager_group.children.push_back(controller.pages[0]);
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
				interactions.emplace<action::selection_t>(view.entity, target);
				break;

			default:

				debug_printline(engine::gui_channel, "Unknown interaction type.");
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

				node->reactions.push_back(reaction_text_t{ view });
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
