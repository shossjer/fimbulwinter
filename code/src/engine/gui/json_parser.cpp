
#include "actions.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "resources.hpp"

#include <engine/debug.hpp>

#include <utility/json.hpp>

#include <fstream>

namespace
{
	using namespace engine::gui;

	using key = std::string;

	struct ResourceLoader
	{
	private:

		const json & jdimensions;
		const json & jstrings;
		const json & jtemplates;

		struct Map
		{
			const json & jdata;

			auto contains(const key key) const { return ::contains(this->jdata, key); }
			auto get(const key key) const { return jdata[key].get<std::string>(); }
		};
		std::vector<Map> overriders;

	public:

		ResourceLoader(const json & jcontent)
			: jdimensions(jcontent["dimensions"])
			, jstrings(jcontent["strings"])
			, jtemplates(jcontent["templates"])
		{
			if (contains(jcontent, "colors"))
			{
				const json & jcolors = jcontent["colors"];

				for (auto i = jcolors.begin(); i != jcolors.end(); ++i)
				{
					const std::string str = i.value();

					debug_assert(str.length() == 10);

					const auto val = std::stoul(str, nullptr, 16);

					resource::put(engine::Asset{ i.key() }, val);
					debug_printline(engine::gui_channel, "GUI - loading color: ", i.key());
				}
			}

			if (contains(jcontent, "selectors"))
			{
				const json & jselectors = jcontent["selectors"];

				for (auto i = jselectors.begin(); i != jselectors.end(); ++i)
				{
					auto & jval = i.value();
					debug_assert(jval.is_object());

					debug_assert(contains(jval, "default"));
					engine::Asset defColor = jval["default"].get<std::string>();

					engine::Asset highColor;
					if (contains(jval, "highlight"))
						highColor = jval["highlight"].get<std::string>();
					else
						highColor = defColor;

					engine::Asset pressedColor;
					if (contains(jval, "pressed"))
						pressedColor = jval["pressed"].get<std::string>();
					else
						pressedColor = defColor;

					resource::put(engine::Asset{ i.key() }, defColor, highColor, pressedColor);
					debug_printline(engine::gui_channel, "GUI - loading selector: ", i.key());
				}
			}
		}

	public:

		void push_override(const json & remap)
		{
			overriders.emplace_back(Map{ remap });
		}
		void pop_override()
		{
			overriders.pop_back();
		}

	private:

		// TODO: make possible to map #key to another #key2
		auto find_override(const key key) const
		{
			for (auto ritr = this->overriders.rbegin(); ritr != this->overriders.rend(); ++ritr)
			{
				if ((*ritr).contains(key))
				{
					auto value = (*ritr).get(key);
					debug_printline(engine::gui_channel, "Re-mapping: ", key, " to: ", value);
					return value;
				}
			}
			if (contains(jstrings, key))
			{
				return jstrings[key].get<std::string>();
			}
			throw key_missing(key);
		}

		template<typename N>
		N number(const json & jv) const
		{
			if (!jv.is_string())
				return jv.get<N>();

			const std::string str = jv;
			debug_assert(str.length() > std::size_t{ 0 });
			debug_assert(str[0] == '#');
			return this->jdimensions[str].get<N>();
		}
		template<typename N>
		N number(const key key, const json & jdata) const
		{
			debug_assert(contains(jdata, key));
			return number<N>(jdata[key]);
		}
		template<typename N>
		N number(const key key, const json & jdata, const N def) const
		{
			if (!contains(jdata, key))
				return def;
			return number<N>(jdata[key]);
		}

		engine::Asset extract_asset(const json & jdata, const key key) const
		{
			debug_assert(contains(jdata, key));
			return engine::Asset{ jdata[key].get<std::string>() };
		}

		template<typename S>
		auto extract_dimen(const json & jd) const
		{
			if (jd.is_object())
			{
				debug_assert(contains(jd, "t"));
				const std::string type = jd["t"];

				if (type == "fixed")
				{
					return Size::extended_dimen_t<S>{ Size::FIXED, S{ number<uint32_t>("v", jd) } };
				}
				else
				if (type == "parent")
				{
					return Size::extended_dimen_t<S>{ Size::PARENT };
				}
				else
				if (type == "percent")
				{
					return Size::extended_dimen_t<S>{ Size::PARENT };
			//		return S{ Size::PERCENT, _int("v", jd, 0) / value_t{100.f} };
				}
				else
				if (type == "wrap")
				{
					return Size::extended_dimen_t<S>{ Size::WRAP, S{ number<uint32_t>("min", jd, 0) } };
				}
			}
			else
			if (jd.is_string())
			{
				const std::string str = jd;

				debug_assert(str.length() > std::size_t{ 0 });

				if (str[0] == '#')
					return Size::extended_dimen_t<S>{
						Size::FIXED,
						S{ this->jdimensions[str].get<uint32_t>() } };

				if (str == "percent") return Size::extended_dimen_t<S>{ Size::PARENT };//Size::TYPE::PERCENT };
				if (str == "parent") return Size::extended_dimen_t<S>{ Size::PARENT };
				if (str == "wrap") return Size::extended_dimen_t<S>{Size::WRAP };

				debug_printline(engine::gui_channel, "GUI - invalid size dimen: ", jd);
				debug_unreachable();
			}
			else
			if (jd.is_number_integer())
			{
				return Size::extended_dimen_t<S>{ Size::FIXED, S{ jd.get<uint32_t>() } };
			}

			debug_printline(engine::gui_channel, "GUI - invalid size value: ", jd);
			debug_unreachable();
		}

		auto extract_size(const json & jdata) const
		{
			const json & jsize = jdata["size"];

			debug_assert(contains(jsize, "h"));
			debug_assert(contains(jsize, "w"));

			return Size{
				extract_dimen<height_t>(jsize["h"]),
				extract_dimen<width_t>(jsize["w"]) };
		}

		auto extract_string(const key key, const json & jdata) const
		{
			debug_assert(contains(jdata, key));
			auto str = jdata[key].get<std::string>();

			if (!str.empty() && str[0] == '#')
			{
				return find_override(str);
			}
			return str;
		}

	public:

		bool has_color(const json & jdata) const
		{
			return contains(jdata, "color");
		}

		engine::Asset color(const json & jdata) const
		{
			debug_assert(contains(jdata, "color"));

			const std::string str = jdata["color"];
			debug_assert(str.length() > std::size_t{ 0 });

			const engine::Asset asset{ str };

			debug_assert(resource::color(asset)!= nullptr);

			return asset;
		}

		const json & components(const json & jdata) const
		{
			debug_assert(contains(jdata, "components"));
			return jdata["components"];
		}

		bool has_display(const json & jdata) const
		{
			return contains(jdata, "display");
		}
		std::string display(const json & jdata) const
		{
			return extract_string("display", jdata);
		}

		View::Group::Layout layout(const json & jgroup) const
		{
			debug_assert(contains(jgroup, "layout"));

			const std::string str = jgroup["layout"];

			if (str == "horizontal")return View::Group::HORIZONTAL;
			if (str == "vertical") return View::Group::VERTICAL;
			if (str == "relative") return View::Group::RELATIVE;

			debug_printline(engine::gui_channel, "GUI - invalid layout: ", str);
			debug_unreachable();
		}

		Gravity gravity(const json & jdata) const
		{
			uint_fast16_t h = Gravity::HORIZONTAL_LEFT;
			uint_fast16_t v = Gravity::VERTICAL_TOP;

			if (contains(jdata, "gravity"))
			{
				const json & jgravity = jdata["gravity"];

				if (contains(jgravity, "h"))
				{
					const std::string str = jgravity["h"];

					if (str == "left") h = Gravity::HORIZONTAL_LEFT;
					else
					if (str == "centre") h = Gravity::HORIZONTAL_CENTRE;
					else
					if (str == "right") h = Gravity::HORIZONTAL_RIGHT;
					else
					{
						debug_printline(engine::gui_channel, "GUI - invalid horizontal gravity: ", str);
						debug_unreachable();
					}
				}

				if (contains(jgravity, "v"))
				{
					const std::string str = jgravity["v"];

					if (str == "top") v = Gravity::VERTICAL_TOP;
					else
					if (str == "centre") v = Gravity::VERTICAL_CENTRE;
					else
					if (str == "bottom") v = Gravity::VERTICAL_BOTTOM;
					else
					{
						debug_printline(engine::gui_channel, "GUI - invalid vertical gravity: ", str);
						debug_unreachable();
					}
				}
			}

			return Gravity{ h | v };
		}

		auto has_name(const json & jdata) const
		{
			return contains(jdata, "name");
		}
		auto name(const json & jdata) const
		{
			auto value = extract_string("name", jdata);
			debug_assert(!value.empty());
			return value;
		}
		auto name_or_empty(const json & jdata) const
		{
			if (has_name(jdata))
			{
				auto value = extract_string("name", jdata);
				debug_assert(!value.empty());
				return value;
			}
			return std::string{};
		}

		Margin margin(const json & jdata) const
		{
			Margin margin;

			if (contains(jdata, "margin"))
			{
				const json & jmargin = jdata["margin"];

				margin.bottom = height_t{ number<uint32_t>("b", jmargin, 0) };
				margin.left = width_t{ number<uint32_t>("l", jmargin, 0) };
				margin.right = width_t{ number<uint32_t>("r", jmargin, 0) };
				margin.top = height_t{ number<uint32_t>("t", jmargin, 0) };
			}

			return margin;
		}

		bool has_texture(const json & jdata) const
		{
			return contains(jdata, "res");
		}
		engine::Asset texture(const json & jdata) const
		{
			return extract_asset(jdata, "res");
		}

		Size size(const json & jdata) const
		{
			debug_assert(contains(jdata, "size"));
			return extract_size(jdata);
		}

		Size size_def_parent(const json & jdata) const
		{
			if (!contains(jdata, "size"))
				return Size{ Size::PARENT, Size::PARENT };
			return extract_size(jdata);
		}

		Size size_def_wrap(const json & jdata) const
		{
			if (!contains(jdata, "size"))
				return Size{ Size::WRAP, Size::WRAP };
			return extract_size(jdata);
		}

		const json & template_data(const key key) const
		{
			debug_assert(contains(this->jtemplates, key));
			return this->jtemplates[key];
		}

		auto has_target(const json & jdata) const
		{
			return contains(jdata, "target");
		}
		auto target(const json & jdata) const
		{
			auto value = extract_string("target", jdata);
			debug_assert(!value.empty());
			return value;
		}

		engine::Asset type(const json & jdata) const
		{
			auto value = extract_string("type", jdata);
			debug_assert(!value.empty());
			return value;
		}

		bool has_type_group(const json & jdata) const
		{
			return contains(jdata, "group");
		}
		const json & type_group(const json & jdata) const
		{
			debug_assert(has_type_group(jdata));
			return jdata["group"];
		}

		bool has_type_list(const json & jdata) const
		{
			return contains(jdata, "list");
		}
		const json & type_list(const json & jdata) const
		{
			debug_assert(has_type_list(jdata));
			return jdata["list"];
		}

		bool has_type_panel(const json & jdata) const
		{
			return contains(jdata, "panel");
		}
		const json & type_panel(const json & jdata) const
		{
			debug_assert(has_type_panel(jdata));
			return jdata["panel"];
		}

		bool has_type_text(const json & jdata) const
		{
			return contains(jdata, "text");
		}
		const json & type_text(const json & jdata) const
		{
			debug_assert(has_type_text(jdata));
			return jdata["text"];
		}

		bool has_type_texture(const json & jdata) const
		{
			return contains(jdata, "texture");
		}
		const json & type_texture(const json & jdata) const
		{
			debug_assert(has_type_texture(jdata));
			return jdata["texture"];
		}

		bool has_observe(const json & jdata) const
		{
			return contains(jdata, "observe");
		}
		auto observe(const json & jdata) const
		{
			debug_assert(has_observe(jdata));
			return extract_string("observe", jdata);
		}
		auto observe_or_empty(const json & jdata) const
		{
			if (!contains(jdata, "observe"))
				return std::string{};

			return extract_string("observe", jdata);
		}
	};

	struct WindowLoader
	{
	private:

		ResourceLoader & load;

	public:

		WindowLoader(ResourceLoader & load)
			: load(load)
		{}

	private:

		bool has_action(const json & jcomponent) { return contains(jcomponent, "actions"); }

		void load_action(ViewData & view, const json & jcomponent)
		{
			if (!has_action(jcomponent))
				return;

			const json & jactions = jcomponent["actions"];

			for (const auto & jaction : jactions)
			{
				view.actions.emplace_back();
				auto & action = view.actions.back();

				action.type = this->load.type(jaction);
				action.target = this->load.has_target(jaction) ? this->load.target(jaction) : engine::Asset::null();

				// validate action
				switch (action.type)
				{
				case engine::Asset{"none"}:

					break;
				case ViewData::Action::CLOSE:

					break;
				case ViewData::Action::INTERACTION:

					break;
				case ViewData::Action::MOVER:

					break;
				case ViewData::Action::SELECT:

					break;
				case ViewData::Action::TRIGGER:
					debug_assert(action.target != engine::Asset::null());
					break;

				default:
					debug_printline(engine::gui_channel, "GUI - unknown trigger action in component: ", jcomponent);
					debug_unreachable();
				}
			}
		}

		bool has_function(const json & jcomponent) { return contains(jcomponent, "function"); }

		void load_function(ViewData & data, const json & jcomponent)
		{
			if (!has_function(jcomponent))
				return;

			const json & jfunction = jcomponent["function"];

			auto & function = data.function;

			function.observe = this->load.observe_or_empty(jfunction);
			function.type = this->load.type(jfunction);

			switch (function.type)
			{
			case ViewData::Function::LIST:
			{
				GroupData * group = dynamic_cast<GroupData*>(&data);

				if (group == nullptr)
				{
					throw bad_json("'List' can only be added to 'group': ", jfunction);
				}

				if (!group->children.empty())
				{
					throw bad_json("Group with 'list' functionality must have empty component list: ", jfunction);
				}

				if (!contains(jfunction, "template"))
				{
					throw bad_json("'List' must have 'template' definition: ", jfunction);
				}
				auto & jtemplate = jfunction["template"];

				load_component(*group, jtemplate);
				function.templates = std::move(group->children);

				debug_assert(function.templates.size() == 1);
				debug_assert(group->children.empty());

				break;
			}
			case ViewData::Function::PROGRESS:
			{
			//	if (contains(jfunction, "direction"))
			//	{
			//		const std::string direction = jfunction["direction"];

			//		if (direction == "horizontal")
			//		{
			//			target.function.direction = ProgressBar::HORIZONTAL;
			//		}
			//		else
			//		if (direction == "vertical")
			//		{
			//			target.function.direction = ProgressBar::VERTICAL;
			//		}
			//		else
			//		{
			//			debug_printline(engine::gui_channel, "GUI - Progress bar invalid direction: ", jcomponent);
			//			debug_unreachable();
			//		}
			//	}
			//	else
			//	{
			//		target.function.direction = ProgressBar::HORIZONTAL;
			//	}
				break;
			}
			case ViewData::Function::TAB:
				break;

			default:
				throw bad_json("Invalid function type: ", jfunction);
			}
		}

		bool has_reaction(const json & jcomponent) { return contains(jcomponent, "reaction"); }

		void load_reaction(ViewData & view, const json & jcomponent)
		{
			if (!has_reaction(jcomponent))
				return;

			const json & jreaction = jcomponent["reaction"];

			debug_assert(contains(jreaction, "observe"));
			view.reaction.observe = jreaction["observe"].get<std::string>();
			// TEMP
			view.reaction.type = ViewData::Reaction::TEXT;
		}

		GroupData & load_group(GroupData & parent, const json & jcomponent)
		{
			const json & jgroup = this->load.type_group(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<GroupData>,
				this->load.name_or_empty(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.layout(jgroup));

			GroupData & view = utility::get<GroupData>(parent.children.back());

			return view;
		}

		PanelData & load_panel(GroupData & parent, const json & jcomponent)
		{
			const json & jpanel = this->load.type_panel(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<PanelData>,
				this->load.name_or_empty(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.color(jpanel));

			PanelData & view = utility::get<PanelData>(parent.children.back());

			return view;
		}

		TextData & load_text(GroupData & parent, const json & jcomponent)
		{
			const json & jtext = this->load.type_text(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<TextData>,
				this->load.name_or_empty(jcomponent),
				this->load.size_def_wrap(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.color(jtext),
				this->load.display(jtext));

			TextData & view = utility::get<TextData>(parent.children.back());

			// to compensate for text height somewhat
			view.margin.top += height_t{ 6 };

			return view;
		}

		TextureData & load_texture(GroupData & parent, const json & jcomponent)
		{
			const json & jtexture = this->load.type_texture(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<TextureData>,
				this->load.name_or_empty(jcomponent),
				this->load.size(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.texture(jtexture));

			TextureData & view = utility::get<TextureData>(parent.children.back());

			return view;
		}

		void load_views(GroupData & parent, const std::string type, const json & jcomponent)
		{
			if (type == "group")
			{
				auto & group = load_group(parent, jcomponent);
				load_components(group, this->load.components(jcomponent));

				load_function(group, jcomponent);
				load_reaction(group, jcomponent);
			}
			else
			if (type == "panel")
			{
				auto & view = load_panel(parent, jcomponent);

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(view, jcomponent);
			}
			else
			if (type == "text")
			{
				auto & view = load_text(parent, jcomponent);

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(view, jcomponent);
			}
			else
			if (type == "texture")
			{
				auto & view = load_texture(parent, jcomponent);

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(view, jcomponent);
			}
		}

		void load_views(GroupData & parent, const json & jcomponent)
		{
			debug_assert(contains(jcomponent, "type"));
			const std::string type = jcomponent["type"];
			debug_assert(!type.empty());

			load_views(parent, type, jcomponent);
		}

		void load_component(GroupData & parent, const json & jcomponent)
		{
			debug_assert(contains(jcomponent, "type"));
			const std::string type = jcomponent["type"];
			debug_assert(!type.empty());

			if (type[0] == '#')
			{
				// the json map can contain "overriding values" for the template view.
				this->load.push_override(jcomponent);
				{
					load_views(parent, this->load.template_data(type));
				}
				this->load.pop_override();
			}
			else
			{
				load_views(parent, type, jcomponent);
			}
		}
		void load_components(GroupData & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				load_component(parent, jcomponent);
			}
		}

	public:

		void create(std::vector<DataVariant> & windows, const json & jwindow)
		{
			const json & jgroup = this->load.type_group(jwindow);

			windows.emplace_back(
				utility::in_place_type<GroupData>,
				this->load.name(jwindow),
				this->load.size(jwindow),
				this->load.margin(jwindow),
				this->load.gravity(jwindow),
				this->load.layout(jgroup));

			GroupData & window = utility::get<GroupData>(windows.back());

			// load the windows components
			load_components(window, this->load.components(jwindow));
		}
	};
}

namespace engine
{
namespace gui
{
	std::vector<DataVariant> load()
	{
		std::vector<DataVariant> windows;

		std::ifstream file("res/gui.json");

		if (!file.is_open())
		{
			debug_printline(engine::gui_channel, "GUI - res/gui.json file is missing. No GUI is loaded.");
			return windows;
		}

		const json jcontent = json::parse(file);

		ResourceLoader resourceLoader(jcontent);

		const auto & jwindows = jcontent["windows"];
		windows.reserve(jwindows.size());

		for (const auto & jwindow : jwindows)
		{
			WindowLoader(resourceLoader).create(windows, jwindow);
		}

		return windows;
	}
}
}
