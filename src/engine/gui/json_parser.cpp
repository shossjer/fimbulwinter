
#include "defines.hpp"

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

namespace engine
{
namespace gui
{
	extern core::container::Collection
		<
		engine::Asset, 201,
		std::array<engine::Entity, 100>,
		std::array<int, 1>
		>
		lookup;
}
}

namespace
{
	using value_t = engine::gui::value_t;

	using Group = engine::gui::Group;
	using PanelC = engine::gui::PanelC;
	using PanelT = engine::gui::PanelT;
	using Text = engine::gui::Text;
	using View = engine::gui::View;
	using Window = engine::gui::Window;

	struct ResourceLoader
	{
	private:
		const json & jcolors;
		const json & jdimensions;
		const json & jstrings;

	public:

		ResourceLoader(const json & jcontent)
			: jcolors(jcontent["colors"])
			, jdimensions(jcontent["dimensions"])
			, jstrings(jcontent["strings"])
		{
			// load colors and themes and strings? into resources...
		}

	private:

		engine::graphics::data::Color extract_color(const json & jdata)
		{
			const std::string str = jdata["color"];
			debug_assert(str.length() > std::size_t{ 0 });

			if (str[0] == '#')
			{
				std::string res = this->jcolors[str];
				debug_assert(res.length() > std::size_t{ 0 });

				return std::stoul(res, nullptr, 16);
			}

			return std::stoul(str, nullptr, 16);
		}

		value_t extract_margin(const std::string key, const json & jmargin)
		{
			if (!contains(jmargin, key))
				return 0;

			const json & jv = jmargin[key];

			if (!jv.is_string())
				return jv.get<value_t>();

			const std::string str = jv;
			debug_assert(str.length() > std::size_t{ 0 });
			debug_assert(str[0] == '#');
			return this->jdimensions[str].get<value_t>();
		}

		View::Size::Dimen extract_dimen(const json & jd)
		{
			if (jd.is_string())
			{
				const std::string str = jd;

				debug_assert(str.length() > std::size_t{ 0 });

				if (str[0] == '#')
					return View::Size::Dimen{
						View::Size::TYPE::FIXED,
						this->jdimensions[str].get<value_t>() };

				if (str[str.length() - 1] == '%')
				{
					value_t val = std::stof(str.substr(0, str.length() - 1));
					return View::Size::Dimen{ View::Size::TYPE::PERCENTAGE, val / value_t{ 100 } };
				}
				if (str == "parent") return View::Size::Dimen{ View::Size::TYPE::PARENT };
				if (str == "wrap") return View::Size::Dimen{ View::Size::TYPE::WRAP };

				debug_printline(0xffffffff, "GUI - invalid size dimen: ", str);
				debug_unreachable();
			}

			return View::Size::Dimen{ View::Size::TYPE::FIXED, jd.get<value_t>() };
		}

		View::Size extract_size(const json & jdata)
		{
			const json jsize = jdata["size"];

			debug_assert(contains(jsize, "w"));
			debug_assert(contains(jsize, "h"));

			return View::Size{ extract_dimen(jsize["w"]), extract_dimen(jsize["h"]) };
		}

	public:

		engine::graphics::data::Color color(const json & jdata)
		{
			debug_assert(contains(jdata, "color"));
			return extract_color(jdata);
		}

		engine::graphics::data::Color color(const json & jdata, const engine::graphics::data::Color def_val)
		{
			if (!contains(jdata, "color")) return def_val;
			return extract_color(jdata);
		}

		Group::Layout layout(const json & jgroup)
		{
			debug_assert(contains(jgroup, "layout"));

			const std::string str = jgroup["layout"];

			if (str == "horizontal")return Group::Layout::HORIZONTAL;
			if (str == "vertical") return Group::Layout::VERTICAL;
			if (str == "relative") return Group::Layout::RELATIVE;

			debug_printline(0xffffffff, "GUI - invalid layout: ", str);
			debug_unreachable();
		}

		View::Gravity gravity(const json & jdata)
		{
			uint_fast16_t h = View::Gravity::HORIZONTAL_LEFT;
			uint_fast16_t v = View::Gravity::VERTICAL_TOP;

			if (contains(jdata, "gravity"))
			{
				const json jgravity = jdata["gravity"];

				if (contains(jgravity, "h"))
				{
					const std::string str = jgravity["h"];

					if (str == "left") h = View::Gravity::HORIZONTAL_LEFT;
					else
					if (str == "centre") h = View::Gravity::HORIZONTAL_CENTRE;
					else
					if (str == "right") h = View::Gravity::HORIZONTAL_RIGHT;
					else
					{
						debug_printline(0xffffffff, "GUI - invalid horizontal gravity: ", str);
						debug_unreachable();
					}
				}

				if (contains(jgravity, "v"))
				{
					const std::string str = jgravity["v"];

					if (str == "top") v = View::Gravity::VERTICAL_TOP;
					else
					if (str == "centre") v = View::Gravity::VERTICAL_CENTRE;
					else
					if (str == "bottom") v = View::Gravity::VERTICAL_BOTTOM;
					else
					{
						debug_printline(0xffffffff, "GUI - invalid vertical gravity: ", str);
						debug_unreachable();
					}
				}
			}

			return View::Gravity{ h | v };
		}

		bool has_name(const json & jdata) { return contains(jdata, "name"); }

		engine::Asset name(const json & jdata)
		{
			std::string str = jdata["name"];
			debug_assert(str.length() > std::size_t{ 0 });
			return engine::Asset{ str };
		}

		View::Margin margin(const json & jdata)
		{
			View::Margin margin;

			if (contains(jdata, "margin"))
			{
				const json jmargin = jdata["margin"];

				margin.bottom = extract_margin("b", jmargin);
				margin.left = extract_margin("l", jmargin);
				margin.right = extract_margin("r", jmargin);
				margin.top = extract_margin("t", jmargin);
			}

			return margin;
		}

		View::Size size(const json & jdata)
		{
			debug_assert(contains(jdata, "size"));
			return extract_size(jdata);
		}

		View::Size size_def_parent(const json & jdata)
		{
			if (!contains(jdata, "size"))
				return View::Size{ View::Size::TYPE::PARENT, View::Size::TYPE::PARENT };
			return extract_size(jdata);
		}

		View::Size size_def_wrap(const json & jdata)
		{
			if (!contains(jdata, "size"))
				return View::Size{ View::Size::TYPE::WRAP, View::Size::TYPE::WRAP };
			return extract_size(jdata);
		}
	};

	struct Components
	{
		Window & window;
		ResourceLoader & load;

		engine::gui::ACTIONS & actions;
		engine::gui::COMPONENTS & components;

		Components(Window & window, ResourceLoader & load, engine::gui::ACTIONS & actions, engine::gui::COMPONENTS & components)
			: window(window)
			, load(load)
			, actions(actions)
			, components(components)
		{}

	private:

		bool has_action(const json & jcomponent) { return contains(jcomponent, "trigger"); }

		void load_action(engine::Entity entity, const json & jcomponent)
		{
			const json & jtrigger = jcomponent["trigger"];

			engine::Asset action { jtrigger["action"].get<std::string>() };

			switch (action)
			{
			case engine::Asset{ "close" }:

				this->actions.emplace<engine::gui::CloseAction>(entity, engine::gui::CloseAction{ this->window.name });
				break;

			case engine::Asset{ "mover" }:

				this->actions.emplace<engine::gui::MoveAction>(entity, engine::gui::MoveAction{ this->window.name });
				break;

			case engine::Asset{ "selectable" }:

				this->actions.emplace<engine::gui::SelectAction>(entity, engine::gui::SelectAction{ this->window.name });
				break;

			default:
				debug_printline(0xffffffff, "GUI - unknown trigger action in component: ", jcomponent);
				debug_unreachable();
			}

			gameplay::gamestate::post_add(entity, this->window.name, action);
		}

		bool has_function(const json & jcomponent) { return contains(jcomponent, "function"); }

		void load_function(engine::gui::View * target, const json & jcomponent)
		{
			const json & jfunction = jcomponent["function"];

			debug_assert(load.has_name(jfunction));
			const engine::Asset name = load.name(jfunction);

			debug_assert(contains(jfunction, "type"));
			engine::Asset type{ jfunction["type"].get<std::string>() };

			const auto entity = engine::Entity::create();

			switch (type)
			{
			case engine::Asset{ "progressBar" }:
			{
				debug_assert(contains(jfunction, "direction"));
				const std::string direction = jfunction["direction"];
				debug_assert(((direction == "vertical") || (direction == "horizontal")));

				this->components.emplace<engine::gui::ProgressBar>(
					entity,
					engine::gui::ProgressBar{
						this->window.name,
						target,
						direction == "horizontal" ?
							engine::gui::ProgressBar::HORIZONTAL : engine::gui::ProgressBar::VERTICAL });
				break;
			}
			default:
				break;
			}

			engine::gui::lookup.emplace<engine::Entity>(name, entity);
		}

		Group & load_group(engine::Entity entity, const json & jcomponent)
		{
			debug_assert(contains(jcomponent, "group"));

			const json jgroup = jcomponent["group"];

			return this->components.emplace<Group>(
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.layout(jgroup));
		}

		Text & load_text(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "text"));

			const json jtext = jcomponent["text"];

			// to compensate for text height somewhat
			View::Margin margin = this->load.margin(jcomponent);
			margin.top += 6;

			return this->components.emplace<Text>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				margin,
				this->load.size_def_wrap(jcomponent),
				this->load.color(jtext, 0xff000000),
				jtext["display"]);
		}

		PanelC & load_panel(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "panel"));

			const json jpanel = jcomponent["panel"];

			return this->components.emplace<PanelC>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.color(jpanel),
				selectable);
		}

		PanelT & load_texture(engine::Entity entity, const json & jcomponent, bool selectable)
		{
			debug_assert(contains(jcomponent, "texture"));

			const json jtexture = jcomponent["texture"];

			return this->components.emplace<PanelT>(
				entity,
				entity,
				this->load.gravity(jcomponent),
				this->load.margin(jcomponent),
				this->load.size(jcomponent),
				jtexture["res"],
				selectable);
		}

		void load_components(Group & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				debug_assert(contains(jcomponent, "type"));

				const auto entity = engine::Entity::create();

				const std::string type = jcomponent["type"];

				View * child;

				if (type == "group")
				{
					auto & group = load_group(entity, jcomponent);
					child = &group;

					load_components(group, jcomponent["components"]);
				}
				else
				{
					const bool selectable = has_action(jcomponent);

					if (type == "panel")
					{
						child = &load_panel(entity, jcomponent, selectable);
					}
					else
					if (type == "texture")
					{
						child = &load_texture(entity, jcomponent, selectable);
					}
					else
					if (type == "text")
					{
						child = &load_text(entity, jcomponent, selectable);
					}
					else
					{
						debug_printline(0xffffffff, "GUI - unknown component type: ", jcomponent);
						debug_unreachable();
					}

					if (selectable)
					{
						load_action(entity, jcomponent);
					}

					// register the component in the "lookup" table (asset -> entity) if it is named.
					if (load.has_name(jcomponent))
					{
						engine::gui::lookup.emplace<engine::Entity>(load.name(jcomponent), entity);
					}
				}

				parent.adopt(child);

				// load the view functionality if any
				if (has_function(jcomponent))
				{
					load_function(child, jcomponent);
				}
			}
		}

	public:

		void setup(const json & jwindow)
		{
			load_components(this->window.group, jwindow["components"]);
		}
	};

	auto & create_window(engine::gui::WINDOWS & windows, ResourceLoader & load, const json & jwindow)
	{
		debug_assert(contains(jwindow, "name"));
		debug_assert(contains(jwindow, "group"));

		const std::string name = jwindow["name"];

		debug_printline(0xffffffff, "GUI - creating window: ", name);

		const engine::Asset asset{ name };

		// TODO: read gravity for window position

		const json jgroup = jwindow["group"];

		auto & window = windows.emplace<Window>(
			asset,
			asset,
			load.size(jwindow),
			load.layout(jgroup),
			load.margin(jwindow));

		return window;
	}
}

namespace engine
{
namespace gui
{
	void load(ACTIONS & actions, COMPONENTS & components, WINDOWS & windows)
	{
		std::ifstream file("res/gui.json");

		if (!file.is_open())
		{
			debug_printline(0xffffffff, "GUI - res/gui.json file is missing. No GUI is loaded.");
			return;
		}

		const json jcontent = json::parse(file);

		ResourceLoader resourceLoader(jcontent);

		const auto & jwindows = jcontent["windows"];

		for (const auto & jwindow : jwindows)
		{
			Window & window = create_window(windows, resourceLoader, jwindow);

			// loads the components of the window
			Components(window, resourceLoader, actions, components).setup(jwindow);

			window.measure_window();
		}
	}
}
}
