
#include "actions.hpp"
#include "functions.hpp"
#include "loading.hpp"
#include "resources.hpp"

#include <engine/graphics/renderer.hpp>

#include <utility/json.hpp>

#include <fstream>

namespace
{
	using namespace engine::gui;

	struct ResourceLoader
	{
	private:

		const json & jdimensions;
		const json & jstrings;
		const json & jtemplates;

	public:

		ResourceLoader(const json & jcontent)
			: jdimensions(jcontent["dimensions"])
			, jstrings(jcontent["strings"])
			, jtemplates(jcontent["templates"])
		{
			if (contains(jcontent, "colors"))
			{
				const json & jcolors = jcontent["colors"];

				for (auto & i = jcolors.begin(); i != jcolors.end(); ++i)
				{
					const std::string str = i.value();

					debug_assert(str.length() == 10);

					const auto val = std::stoul(str, nullptr, 16);

					resource::put(engine::Asset{ i.key() }, val);
					debug_printline(0xffffffff, "GUI - loading color: ", i.key());
				}
			}

			if (contains(jcontent, "selectors"))
			{
				const json & jselectors = jcontent["selectors"];

				for (auto & i = jselectors.begin(); i != jselectors.end(); ++i)
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
					debug_printline(0xffffffff, "GUI - loading selector: ", i.key());
				}
			}
		}

	private:

		value_t _int(const json & jv) const
		{
			if (!jv.is_string())
				return jv.get<value_t>();

			const std::string str = jv;
			debug_assert(str.length() > std::size_t{ 0 });
			debug_assert(str[0] == '#');
			return this->jdimensions[str].get<value_t>();
		}
		value_t _int(const std::string key, const json & jdata) const
		{
			debug_assert(contains(jdata, key));
			return _int(jdata[key]);
		}
		value_t _int(const std::string key, const json & jdata, const value_t def) const
		{
			if (!contains(jdata, key))
				return def;
			return _int(jdata[key]);
		}

		engine::Asset extract_asset(const json & jdata, const std::string key) const
		{
			debug_assert(contains(jdata, key));
			return engine::Asset{ jdata[key].get<std::string>() };
		}

		Size::Dimen extract_dimen(const json & jd) const
		{
			if (jd.is_object())
			{
				debug_assert(contains(jd, "t"));
				const std::string type = jd["t"];

				if (type == "fixed")
				{
					return Size::Dimen{ Size::TYPE::FIXED, _int("v", jd) };
				}
				else
				if (type == "parent")
				{
					return Size::Dimen{ Size::TYPE::PARENT };
				}
				else
				if (type == "percent")
				{
					return Size::Dimen{ Size::TYPE::PERCENT, _int("v", jd, 0) / value_t{100.f} };
				}
				else
				if (type == "wrap")
				{
					return Size::Dimen{ Size::TYPE::WRAP, _int("min", jd, 0) };
				}
			}
			else
			if (jd.is_string())
			{
				const std::string str = jd;

				debug_assert(str.length() > std::size_t{ 0 });

				if (str[0] == '#')
					return Size::Dimen{
						Size::TYPE::FIXED,
						this->jdimensions[str].get<value_t>() };

				if (str == "percent")return Size::Dimen{ Size::TYPE::PERCENT };
				if (str == "parent") return Size::Dimen{ Size::TYPE::PARENT };
				if (str == "wrap") return Size::Dimen{ Size::TYPE::WRAP };

				debug_printline(0xffffffff, "GUI - invalid size dimen: ", jd);
				debug_unreachable();
			}
			else
			if (jd.is_number_integer())
			{
				return Size::Dimen{ Size::TYPE::FIXED, jd.get<value_t>() };
			}

			debug_printline(0xffffffff, "GUI - invalid size value: ", jd);
			debug_unreachable();
		}

		Size extract_size(const json & jdata) const
		{
			const json & jsize = jdata["size"];

			debug_assert(contains(jsize, "w"));
			debug_assert(contains(jsize, "h"));

			return Size{ extract_dimen(jsize["w"]), extract_dimen(jsize["h"]) };
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
			debug_assert(has_display(jdata));
			return jdata["display"].get<std::string>();
		}

		Layout layout(const json & jgroup) const
		{
			debug_assert(contains(jgroup, "layout"));

			const std::string str = jgroup["layout"];

			if (str == "horizontal")return Layout::HORIZONTAL;
			if (str == "vertical") return Layout::VERTICAL;
			if (str == "relative") return Layout::RELATIVE;

			debug_printline(0xffffffff, "GUI - invalid layout: ", str);
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
						debug_printline(0xffffffff, "GUI - invalid horizontal gravity: ", str);
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
						debug_printline(0xffffffff, "GUI - invalid vertical gravity: ", str);
						debug_unreachable();
					}
				}
			}

			return Gravity{ h | v };
		}

		bool has_name(const json & jdata) const
		{
			return contains(jdata, "name");
		}

		std::string name(const json & jdata) const
		{
			debug_assert(has_name(jdata));
			std::string str = jdata["name"];
			debug_assert(str.length() > std::size_t{ 0 });
			return str;
		}

		std::string name_or_null(const json & jdata) const
		{
			if (has_name(jdata))
			{
				std::string str = jdata["name"];
				debug_assert(str.length() > std::size_t{ 0 });
				return str;
			}
			return "";
		}

		Margin margin(const json & jdata) const
		{
			Margin margin;

			if (contains(jdata, "margin"))
			{
				const json & jmargin = jdata["margin"];

				margin.bottom = _int("b", jmargin, 0);
				margin.left = _int("l", jmargin, 0);
				margin.right = _int("r", jmargin, 0);
				margin.top = _int("t", jmargin, 0);
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
				return Size{ Size::TYPE::PARENT, Size::TYPE::PARENT };
			return extract_size(jdata);
		}

		Size size_def_wrap(const json & jdata) const
		{
			if (!contains(jdata, "size"))
				return Size{ Size::TYPE::WRAP, Size::TYPE::WRAP };
			return extract_size(jdata);
		}

		const json & template_data(const std::string key) const
		{
			debug_assert(contains(this->jtemplates, key));
			return this->jtemplates[key];
		}

		engine::Asset type(const json & jdata) const
		{
			debug_assert(contains(jdata, "type"));
			return engine::Asset{ jdata["type"].get<std::string>() };
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
			const json & jactions = jcomponent["actions"];

			for (const auto & jaction : jactions)
			{
				view.actions.emplace_back();
				auto & action = view.actions.back();

				action.type = this->load.type(jaction);
				action.target = contains(jaction, "target") ? jaction["target"].get<std::string>() : engine::Asset::null();

				// validate action
				switch (action.type)
				{
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
					debug_printline(0xffffffff, "GUI - unknown trigger action in component: ", jcomponent);
					debug_unreachable();
				}
			}
		}

		bool has_function(const json & jcomponent) { return contains(jcomponent, "function"); }

		void load_function(ViewData & target, const json & jcomponent)
		{
			const json & jfunction = jcomponent["function"];

			target.function.name = this->load.name(jfunction);
			target.function.type = this->load.type(jfunction);

			switch (target.function.type)
			{
			case ViewData::Function::PROGRESS:
			{
				if (contains(jfunction, "direction"))
				{
					const std::string direction = jfunction["direction"];

					if (direction == "horizontal")
					{
						target.function.direction = ProgressBar::HORIZONTAL;
					}
					else
					if (direction == "vertical")
					{
						target.function.direction = ProgressBar::VERTICAL;
					}
					else
					{
						debug_printline(0xffffffff, "GUI - Progress bar invalid direction: ", jcomponent);
						debug_unreachable();
					}
				}
				else
				{
					target.function.direction = ProgressBar::HORIZONTAL;
				}
				break;
			}
			case ViewData::Function::TAB:
			{
				break;
			}
			default:
				debug_printline(0xffffffff, "GUI - Unknown function type: ", jcomponent);
				debug_unreachable();
			}
		}

		GroupData & load_group(GroupData & parent, const json & jcomponent)
		{
			const json & jgroup = this->load.type_group(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<GroupData>,
				this->load.name_or_null(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.layout(jgroup));

			GroupData & view = utility::get<GroupData>(parent.children.back());

			return view;
		}

		ListData & load_list(GroupData & parent, const json & jcomponent)
		{
			const json & jgroup = this->load.type_list(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<ListData>,
				this->load.name(jcomponent),
				this->load.size_def_parent(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.layout(jgroup));

			ListData & view = utility::get<ListData>(parent.children.back());

			return view;
		}

		PanelData & load_panel(GroupData & parent, const json & jcomponent)
		{
			const json & jpanel = this->load.type_panel(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<PanelData>,
				this->load.name_or_null(jcomponent),
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
				this->load.name_or_null(jcomponent),
				this->load.size_def_wrap(jcomponent),
				this->load.margin(jcomponent),
				this->load.gravity(jcomponent),
				this->load.color(jtext),
				this->load.display(jtext));

			TextData & view = utility::get<TextData>(parent.children.back());

			// to compensate for text height somewhat
			view.margin.top += 6;

			return view;
		}

		TextureData & load_texture(GroupData & parent, const json & jcomponent)
		{
			const json & jtexture = this->load.type_texture(jcomponent);

			parent.children.emplace_back(
				utility::in_place_type<TextureData>,
				this->load.name_or_null(jcomponent),
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

				if (has_function(jcomponent))
				{
					load_function(group, jcomponent);
				}
			}
			else
			if (type == "list")
			{
				auto & list = load_list(parent, jcomponent);
				const json & jitem_template = this->load.components(jcomponent);
				debug_assert(jitem_template.size()== 1);
				load_components(list, jitem_template);
			}
			else
			if (type == "panel")
			{
				auto & view = load_panel(parent, jcomponent);

				if (has_action(jcomponent))
				{
					load_action(view, jcomponent);
				}
				if (has_function(jcomponent))
				{
					load_function(view, jcomponent);
				}
			}
			else
			if (type == "text")
			{
				auto & view = load_text(parent, jcomponent);

				if (has_action(jcomponent))
				{
					load_action(view, jcomponent);
				}
				if (has_function(jcomponent))
				{
					load_function(view, jcomponent);
				}
			}
			else
			if (type == "texture")
			{
				auto & view = load_texture(parent, jcomponent);

				if (has_action(jcomponent))
				{
					load_action(view, jcomponent);
				}
				if (has_function(jcomponent))
				{
					load_function(view, jcomponent);
				}
			}
		}

		void load_views(GroupData & parent, const json & jcomponent)
		{
			debug_assert(contains(jcomponent, "type"));
			const std::string type = jcomponent["type"];
			debug_assert(!type.empty());

			load_views(parent, type, jcomponent);
		}

		struct Customize
		{
			const ResourceLoader & load;
			const json & jdata;

			std::unordered_map<std::string, std::string> customized_names;

		private:

			// checks if the view has a custom definition in the implementation of the view.
			bool is_customized(ViewData & data)
			{
				return data.has_name() && contains(jdata, data.name);
			}

			const json & customize_common(ViewData & data)
			{
				const json & jcustomize = this->jdata[data.name];

				// customize the name and register the change.
				if (this->load.has_name(jcustomize))
				{
					const std::string name = this->load.name(jcustomize);
					customized_names.insert_or_assign(data.name, name);
					data.name = name;
				}

				return jcustomize;
			}

			// actions can have "named targets" make sure to update the name
			// if it has been "customized".
			void update_actions(ViewData & data)
			{
				for (auto & action : data.actions)
				{
					for (const auto & d : customized_names)
					{
						// not optimal...
						if (engine::Asset{ d.first } == action.target)
						{
							action.target = engine::Asset{ d.second };
							return;
						}
					}
				}
			}

		public:

			void operator() (GroupData & data)
			{
				update_actions(data);

				if (is_customized(data))
				{
					customize_common(data);
				}

				// customize any template children of the group
				for (auto & child : data.children)
				{
					visit(*this, child);
				}
			}
			void operator() (ListData & data)
			{
				update_actions(data);

				if (is_customized(data))
				{
					customize_common(data);
				}

				// customize the list item template
				for (auto & child : data.children)
				{
					visit(*this, child);
				}
			}
			void operator() (PanelData & data)
			{
				update_actions(data);

				if (is_customized(data))
				{
					const json & jcustomize = customize_common(data);

					if (this->load.has_type_panel(jcustomize))
					{
						const json & jpanel = this->load.type_panel(jcustomize);

						if (this->load.has_color(jpanel))
							data.color = this->load.color(jpanel);
					}
				}
			}
			void operator() (TextData & data)
			{
				update_actions(data);

				if (is_customized(data))
				{
					const json & jcustomize = customize_common(data);

					if (this->load.has_type_text(jcustomize))
					{
						const json & jtext = this->load.type_text(jcustomize);

						if (this->load.has_display(jtext))
							data.display = this->load.display(jtext);

						if (this->load.has_color(jtext))
							data.color = this->load.color(jtext);
					}
				}
			}
			void operator() (TextureData & data)
			{
				update_actions(data);

				if (is_customized(data))
				{
					const json & jcustomize = customize_common(data);

					if (this->load.has_type_texture(jcustomize))
					{
						const json & jtexture = this->load.type_texture(jcustomize);

						if (this->load.has_texture(jtexture))
							data.texture = this->load.texture(jtexture);
					}
				}
			}
		};

		void load_components(GroupData & parent, const json & jcomponents)
		{
			for (const auto & jcomponent : jcomponents)
			{
				debug_assert(contains(jcomponent, "type"));
				const std::string type = jcomponent["type"];
				debug_assert(!type.empty());

				if (type[0] == '#')
				{
					load_views(parent, this->load.template_data(type));
					DataVariant & datav = parent.children.back();
					visit(Customize{ this->load, jcomponent }, datav);
				}
				else
				{
					load_views(parent, type, jcomponent);
				}
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
				Gravity(),
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
	void load(std::vector<DataVariant> & windows)
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
		windows.reserve(jwindows.size());

		for (const auto & jwindow : jwindows)
		{
			WindowLoader(resourceLoader).create(windows, jwindow);
		}
	}
}
}
