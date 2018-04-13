
#include "exception.hpp"
#include "loading.hpp"
#include "loading_utils.hpp"
#include "resources.hpp"

#include <engine/debug.hpp>

#include <utility/json.hpp>

#include <fstream>
#include <unordered_map>

namespace
{
	using namespace engine::gui;

	inline bool is_resource(const std::string & value)
	{
		return !value.empty() && value[0] == '#';
	}

	inline void assert_exist(const json & jd, const std::string & key)
	{
		if (!contains(jd, key))
		{
			debug_printline(engine::gui_channel, "WARNING - key: ", key, " is missing from json: ", jd);
			throw key_missing{ key };
		}
	}

	inline std::string extract_string(const json & jd, const std::string & key)
	{
		return jd[key].get<std::string>();
	}

	inline std::string extract_string_or_res(const json & jd, const std::string & key, const ResourceLoader & rl)
	{
		auto str = jd[key].get<std::string>();

		return is_resource(str) ? rl.string(key) : str;
	}

	template<typename S>
	auto extract_dimen(const ResourceLoader & rl, const json & jd)
	{
		if (jd.is_object())
		{
			assert_exist(jd, "t");

			const std::string type = jd["t"];

			if (type == "fixed")
			{
				return Size::extended_dimen_t<S>{ Size::FIXED, S{ rl.number<uint32_t>("v", jd) } };
			}
			else if (type == "parent")
			{
				return Size::extended_dimen_t<S>{ Size::PARENT };
			}
			else if (type == "percent")
			{
				return Size::extended_dimen_t<S>{ Size::PARENT };
				//		return S{ Size::PERCENT, _int("v", jd, 0) / value_t{100.f} };
			}
			else if (type == "wrap")
			{
				return Size::extended_dimen_t<S>{ Size::WRAP, S{ rl.number<uint32_t>("min", jd, 0) } };
			}
		}
		else if (jd.is_string())
		{
			const std::string str = jd;

			if (str.empty())
			{
				debug_printline(engine::gui_channel, "WARNING - dimention json invalid: ", jd);
				throw bad_json();
			}

			// TODO: fix
			if (str[0] == '#')
			{
				return Size::extended_dimen_t<S>{
					Size::FIXED,
						S{ rl.jdimensions[str].get<uint32_t>() } };
			}

			if (str == "percent") return Size::extended_dimen_t<S>{ Size::PARENT };//Size::TYPE::PERCENT };
			if (str == "parent") return Size::extended_dimen_t<S>{ Size::PARENT };
			if (str == "wrap") return Size::extended_dimen_t<S>{ Size::WRAP };

			debug_printline(engine::gui_channel, "WARNING - invalid size dimen: ", jd);
			throw bad_json();
		}
		else if (jd.is_number_integer())
		{
			return Size::extended_dimen_t<S>{ Size::FIXED, S{ jd.get<uint32_t>() } };
		}

		debug_printline(engine::gui_channel, "WARNING - invalid size value: ", jd);
		throw bad_json();
	}
	auto extract_size(const ResourceLoader & rl, const json & jdata)
	{
		const json & jsize = jdata["size"];

		assert_exist(jsize, "h");
		assert_exist(jsize, "w");

		return Size{
			extract_dimen<height_t>(rl, jsize["h"]),
			extract_dimen<width_t>(rl, jsize["w"]) };
	}

	auto parse_color(const ResourceLoader & rl, const json & jdata)
	{
		assert_exist(jdata, "color");

		const std::string str = jdata["color"];

		if (str.empty())
		{
			debug_printline(engine::gui_channel, "WARNING - color value must not be empty", jdata);
			throw bad_json();
		}

		const engine::Asset asset{ str };

		if (resource::color(asset) == nullptr)
		{
			debug_printline(engine::gui_channel, "color is missing from resources", jdata);
			throw key_missing(str);
		}

		return asset;
	}

	const json & parse_content(const json & jd, const std::string & key)
	{
		assert_exist(jd, key);

		auto & content = jd[key];
		debug_assert(content.is_object());
		return content;
	}

	auto parse_gravity(const ResourceLoader & rl, const json & jdata)
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
				else if (str == "centre") h = Gravity::HORIZONTAL_CENTRE;
				else if (str == "right") h = Gravity::HORIZONTAL_RIGHT;
				else
				{
					debug_printline(engine::gui_channel, "GUI - invalid horizontal gravity: ", str);
					throw bad_json();
				}
			}

			if (contains(jgravity, "v"))
			{
				const std::string str = jgravity["v"];

				if (str == "top") v = Gravity::VERTICAL_TOP;
				else if (str == "centre") v = Gravity::VERTICAL_CENTRE;
				else if (str == "bottom") v = Gravity::VERTICAL_BOTTOM;
				else
				{
					debug_printline(engine::gui_channel, "GUI - invalid vertical gravity: ", str);
					throw bad_json();
				}
			}
		}

		return Gravity{ h | v };
	}

	auto parse_layout(const ResourceLoader & rl, const json & jgroup)
	{
		if (!contains(jgroup, "layout"))
		{
			debug_printline(engine::gui_channel, "WARNING - key: 'layout' missing in data: ", jgroup);
			throw key_missing("layout");
		}

		const std::string str = jgroup["layout"];

		if (str == "horizontal") return Layout::HORIZONTAL;
		if (str == "vertical") return Layout::VERTICAL;
		if (str == "relative") return Layout::RELATIVE;

		debug_printline(engine::gui_channel, "GUI - invalid layout: ", str);
		throw bad_json();
	}

	auto parse_margin(const ResourceLoader & rl, const json & jdata)
	{
		Margin margin;

		if (contains(jdata, "margin"))
		{
			const json & jmargin = jdata["margin"];

			margin.bottom = height_t{ rl.number<uint32_t>("b", jmargin, 0) };
			margin.left = width_t{ rl.number<uint32_t>("l", jmargin, 0) };
			margin.right = width_t{ rl.number<uint32_t>("r", jmargin, 0) };
			margin.top = height_t{ rl.number<uint32_t>("t", jmargin, 0) };
		}

		return margin;
	}

	auto parse_size(const ResourceLoader & rl, const json & jdata)
	{
		if (!contains(jdata, "size"))
		{
			return Size{};
		}

		return extract_size(rl, jdata);
	}

	std::string parse_type(const json & jd)
	{
		return jd["type"].get<std::string>();
	}

	class WindowLoader
	{
		const ResourceLoader & rl;

		const json & jtemplates;

	public:

		WindowLoader(ResourceLoader & rl, const json & jtemplates)
			: rl(rl)
			, jtemplates(jtemplates)
		{}

	public:

		void inflate_base(ViewData & d, const json & jd) const
		{
			if (contains(jd, "name"))
			{
				d.name = extract_string_or_res(jd, "name", rl);
			}
			if (contains(jd, "gravity"))
			{
				d.gravity = parse_gravity(rl, jd);
			}
			if (contains(jd, "margin"))
			{
				d.margin = parse_margin(rl, jd);
			}
			if (contains(jd, "size"))
			{
				d.size = parse_size(rl, jd);
			}
		}
		void inflate(GroupData & d, const json & jvd, const json & jtd) const
		{
			if (contains(jtd, "layout"))
			{
				d.layout = parse_layout(rl, jtd);
			}
			inflate_base(d, jvd);
		}
		void inflate(PanelData & d, const json & jvd, const json & jtd) const
		{
			if (contains(jtd, "color"))
			{
				d.color = parse_color(rl, jtd);
			}
			inflate_base(d, jvd);
		}
		void inflate(TextData & d, const json & jvd, const json & jtd) const
		{
			if (contains(jtd, "color"))
			{
				d.color = parse_color(rl, jtd);
			}
			if (contains(jtd, "display"))
			{
				d.display = extract_string_or_res(jtd, "display", rl);
			}
			inflate_base(d, jvd);
		}
		void inflate(TextureData & d, const json & jvd, const json & jtd) const
		{
			if (contains(jtd, "res"))
			{
				d.texture = engine::Asset{ extract_string_or_res(jtd, "res", rl) };
			}
			inflate_base(d, jvd);
		}

		void create(std::vector<DataVariant> & windows, const json & jwindow)
		{
			const json & jgroup = parse_content(jwindow, "group");

			windows.emplace_back(utility::in_place_type<GroupData>);

			GroupData & window = utility::get<GroupData>(windows.back());

			inflate(window, jwindow, jgroup);

			// load the windows components
			load_components(window, jwindow["components"]);
		}

	private:

		void load_action(ViewData & view, const json & jcomponent)
		{
			//if (!contains(jcomponent, "actions"))
			//	return;

			//const json & jactions = jcomponent["actions"];

			//for (const auto & jaction : jactions)
			//{
			//	view.actions.emplace_back();
			//	auto & action = view.actions.back();

			//	action.type = this->load.type(jaction);
			//	action.target = this->load.has_target(jaction) ? this->load.target(jaction) : engine::Asset::null();

			//	// validate action
			//	switch (action.type)
			//	{
			//	case engine::Asset{"none"}:

			//		break;
			//	case ViewData::Action::CLOSE:

			//		break;
			//	case ViewData::Action::INTERACTION:

			//		break;
			//	case ViewData::Action::MOVER:

			//		break;
			//	case ViewData::Action::SELECT:

			//		break;
			//	case ViewData::Action::TRIGGER:
			//		if (action.target == engine::Asset::null())
			//		{
			//			debug_printline(engine::gui_channel, "WARNING - cannot find 'target' of action: ", jcomponent);
			//			throw bad_json();
			//		}
			//		break;

			//	default:
			//		debug_printline(engine::gui_channel, "GUI - unknown trigger action in component: ", jcomponent);
			//		throw bad_json();
			//	}
			//}
		}

		void load_function(ViewData & data, const json & jcomponent)
		{
			//if (!contains(jcomponent, "function"))
			//	return;

			//const json & jfunction = jcomponent["function"];

			//auto & function = data.function;

			//function.type = this->load.type(jfunction);
			//load_reaction(function.reaction, jfunction);

			//switch (function.type)
			//{
			//case ViewData::Function::LIST:
			//{
			//	GroupData * group = dynamic_cast<GroupData*>(&data);

			//	if (group == nullptr)
			//	{
			//		throw bad_json("'List' can only be added to 'group': ", jfunction);
			//	}

			//	if (!group->children.empty())
			//	{
			//		throw bad_json("Group with 'list' functionality must have empty component list: ", jfunction);
			//	}

			//	if (!contains(jfunction, "template"))
			//	{
			//		throw bad_json("'List' must have 'template' definition: ", jfunction);
			//	}
			//	auto & jtemplate = jfunction["template"];

			//	load_component(*group, jtemplate);
			//	function.templates = std::move(group->children);

			//	if (function.templates.size() != 1)
			//	{
			//		debug_printline(engine::gui_channel, "WARNING - 'list' functionality must have exactly 1 'template': ", jcomponent);
			//		throw bad_json();
			//	}
			//	if (!group->children.empty())
			//	{
			//		debug_printline(engine::gui_channel, "WARNING - group with 'list' functionality must be empty: ", jcomponent);
			//		throw bad_json();
			//	}

			//	break;
			//}
			//case ViewData::Function::PROGRESS:
			//{
			////	if (contains(jfunction, "direction"))
			////	{
			////		const std::string direction = jfunction["direction"];
			////		if (direction == "horizontal")
			////		{
			////			target.function.direction = ProgressBar::HORIZONTAL;
			////		}
			////		else
			////		if (direction == "vertical")
			////		{
			////			target.function.direction = ProgressBar::VERTICAL;
			////		}
			////		else
			////		{
			////			debug_printline(engine::gui_channel, "GUI - Progress bar invalid direction: ", jcomponent);
			////			throw bad_json();
			////		}
			////	}
			////	else
			////	{
			////		target.function.direction = ProgressBar::HORIZONTAL;
			////	}
			//	break;
			//}
			//case ViewData::Function::TAB:
			//	break;

			//default:
			//	throw bad_json("Invalid function type: ", jfunction);
			//}
		}

		//void load_reaction(std::vector<ViewData::React> & reaction, const json & jcomponent)
		void load_reaction(const json & jcomponent)
		{
			//if (!contains(jcomponent, "reaction"))
			//	return;

			//const json & jreaction = jcomponent["reaction"];

			//std::stringstream stream{ this->load.observe(jreaction) };
			//std::string node;

			//while (std::getline(stream, node, '.'))
			//{
			//	if (node.empty())
			//	{
			//		debug_printline(engine::gui_channel, "WARNING - key: 'observe' contains empty node: ", jreaction);
			//		throw bad_json();
			//	}

			//	reaction.push_back(ViewData::React{ engine::Asset{ node } });
			//}
		}

		GroupData & load_group(GroupData & parent, const json & jcomponent, const json & jgroup)
		{
			parent.children.emplace_back(utility::in_place_type<GroupData>);

			GroupData & view = utility::get<GroupData>(parent.children.back());

			inflate(view, jcomponent, jgroup);

			return view;
		}

		PanelData & load_panel(GroupData & parent, const json & jcomponent, const json & jpanel)
		{
			parent.children.emplace_back(utility::in_place_type<PanelData>);

			PanelData & view = utility::get<PanelData>(parent.children.back());

			inflate(view, jcomponent, jpanel);

			return view;
		}

		TextData & load_text(GroupData & parent, const json & jcomponent, const json & jtext)
		{
			parent.children.emplace_back(utility::in_place_type<TextData>);

			TextData & view = utility::get<TextData>(parent.children.back());

			inflate(view, jcomponent, jtext);

			// to compensate for text height somewhat
			view.margin.top += height_t{ 6 };

			return view;
		}

		TextureData & load_texture(GroupData & parent, const json & jcomponent, const json & jtexture)
		{
			parent.children.emplace_back(utility::in_place_type<TextureData>);

			TextureData & view = utility::get<TextureData>(parent.children.back());

			inflate(view, jcomponent, jtexture);

			return view;
		}

		void load_views(GroupData & parent, const std::string & type, const json & jcomponent)
		{
			if (type == "group")
			{
				auto & group = load_group(parent, jcomponent, parse_content(jcomponent, type));
				load_components(group, jcomponent["components"]);

				load_function(group, jcomponent);
				load_reaction(jcomponent);
			}
			else if (type == "panel")
			{
				auto & view = load_panel(parent, jcomponent, parse_content(jcomponent, type));

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(jcomponent);
			}
			else if (type == "text")
			{
				auto & view = load_text(parent, jcomponent, parse_content(jcomponent, type));

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(jcomponent);
			}
			else if (type == "texture")
			{
				auto & view = load_texture(parent, jcomponent, parse_content(jcomponent, type));

				load_action(view, jcomponent);
				load_function(view, jcomponent);
				load_reaction(jcomponent);
			}
			else if (is_resource(type))
			{
				load_template(parent, jcomponent, type);
			}
			else
			{
				debug_printline(engine::gui_channel, "WARNING - unrecognized type: ", type);
			}
		}
		void load_views(GroupData & parent, const json & jcomponent)
		{
			load_views(parent, parse_type(jcomponent), jcomponent);
		}

		void load_template(GroupData & parent, const json & jcomponent, const std::string & type)
		{
			assert_exist(this->jtemplates, type);
			load_views(parent, this->jtemplates[type]);

			if (!contains(jcomponent, "override"))
				return;

			const json & joverriders = jcomponent["override"];

			// update overriding names
			for (const json & jo : joverriders)
			{
				assert_exist(jo, "target");

				struct
				{
					const WindowLoader & loader;
					const std::string target;
					const json & jo;

					bool is_match(const ViewData & data) const
					{
						return data.name == target;
					}

					bool operator() (GroupData & data) const
					{
						if (is_match(data))
						{
							loader.inflate(data, jo, parse_content(jo, "group"));
							return true;
						}
						for (auto & child : data.children)
						{
							if (visit(*this, child))
								return true;
						}
						return false;
					}
					bool operator() (PanelData & data) const
					{
						if (is_match(data))
						{
							loader.inflate(data, jo, parse_content(jo, "panel"));
							return true;
						}
						return false;
					}
					bool operator() (TextData & data) const
					{
						if (is_match(data))
						{
							loader.inflate(data, jo, parse_content(jo, "text"));
							return true;
						}
						return false;
					}
					bool operator() (TextureData & data) const
					{
						if (is_match(data))
						{
							loader.inflate(data, jo, parse_content(jo, "texture"));
							return true;
						}
						return false;
					}
				}
				overrider{ *this, extract_string(jo, "target"), jo };
				visit(overrider, parent.children.back());
			}
		}

		void load_components(GroupData & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				load_views(parent, jcomponent);
			}
		}
	};

	void load_colors(const json & jcontent)
	{
		// TODO: replace with resource instance
		if (contains(jcontent, "colors"))
		{
			const json & jcolors = jcontent["colors"];

			for (auto i = jcolors.begin(); i != jcolors.end(); ++i)
			{
				const std::string str = i.value();

				if (str.length() != 10)
				{
					debug_printline(engine::gui_channel, "WARNING: color not valid format: ", str, " - should be \"0xAABBGGRR\"");
					continue;
				}

				try
				{
					const auto val = std::stoul(str, nullptr, 16);

					resource::put(engine::Asset{ i.key() }, val);
				}
				catch (const std::invalid_argument &)
				{
					debug_printline(engine::gui_channel, "WARNING: color not valid format: ", str, " - should be \"0xAABBGGRR\"");
				}
			}
		}

		if (contains(jcontent, "selectors"))
		{
			const json & jselectors = jcontent["selectors"];

			for (auto i = jselectors.begin(); i != jselectors.end(); ++i)
			{
				auto & jval = i.value();

				if (!jval.is_object())
				{
					debug_printline(engine::gui_channel, "WARNING: selector must be json object: ", jval);
					continue;
				}

				if (!contains(jval, "default"))
				{
					debug_printline(engine::gui_channel, "WARNING: selector must contain \"default\": ", jval);
					continue;
				}

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
			}
		}
	}

	json load_file(const char * filename)
	{
		std::ifstream file(filename);

		if (!file.is_open())
		{
			debug_printline(engine::gui_channel, "GUI - ", filename, " file is missing.");
			return json::object();
		}

		json jcontent;

		try
		{
			jcontent = json::parse(file);
		}
		catch (const std::invalid_argument & e)
		{
			debug_printline(engine::gui_channel, "json file: ", filename, " is corrupt", e.what());
			return json::object();
		}

		return jcontent;
	}
}

namespace engine
{
	namespace gui
	{
		std::vector<DataVariant> load()
		{
			// load resources from file and add to GUI
			auto jresources = load_file("res/gui/resources.json");
			ResourceLoader res_loader(jresources);

			load_colors(jresources);

			// load all view templates for access during window loading
			auto jtemplates = load_file("res/gui/templates.json");

			auto jwindows = load_file("res/gui/windows.json"); //const auto & jwindows = jcontent["windows"];


			std::vector<DataVariant> windows;
			windows.reserve(jwindows.size());

			for (const auto & jwindow : jwindows)
			{
				try
				{
					WindowLoader{ res_loader, jtemplates }.create(windows, jwindow);
				}
				catch (const key_missing & e)
				{
					debug_printline(engine::gui_channel, "WARNING - window creation was aborted due to missing data.", e.message);
				}
				catch (const bad_json & e)
				{
					debug_printline(engine::gui_channel, "WARNING - window creation was aborted.", e.message);
				}
			}

			return windows;
		}
	}
}
