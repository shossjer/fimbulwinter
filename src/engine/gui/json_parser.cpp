
#include "actions.hpp"
#include "functions.hpp"
#include "loading.hpp"

#include <engine/graphics/renderer.hpp>

#include <utility/json.hpp>

#include <fstream>

namespace
{
	using namespace engine::gui;

	struct ResourceLoader
	{
	private:
		const json & jcolors;
		const json & jdimensions;
		const json & jstrings;
		const json & jtemplates;

	public:

		ResourceLoader(const json & jcontent)
			: jcolors(jcontent["colors"])
			, jdimensions(jcontent["dimensions"])
			, jstrings(jcontent["strings"])
			, jtemplates(jcontent["templates"])
		{
			// load colors and themes and strings? into resources...
		}

	private:

		engine::Asset extract_asset(const json & jdata, const std::string key) const
		{
			debug_assert(contains(jdata, key));
			return engine::Asset{ jdata[key].get<std::string>() };
		}

		engine::graphics::data::Color extract_color(const json & jdata) const
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

		value_t extract_margin(const std::string key, const json & jmargin) const
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

		Size::Dimen extract_dimen(const json & jd) const
		{
			if (jd.is_string())
			{
				const std::string str = jd;

				debug_assert(str.length() > std::size_t{ 0 });

				if (str[0] == '#')
					return Size::Dimen{
						Size::TYPE::FIXED,
						this->jdimensions[str].get<value_t>() };

				if (str[str.length() - 1] == '%')
				{
					value_t val = std::stof(str.substr(0, str.length() - 1));
					return Size::Dimen{ Size::TYPE::PERCENTAGE, val / value_t{ 100 } };
				}
				if (str == "parent") return Size::Dimen{ Size::TYPE::PARENT };
				if (str == "wrap") return Size::Dimen{ Size::TYPE::WRAP };

				debug_printline(0xffffffff, "GUI - invalid size dimen: ", str);
				debug_unreachable();
			}

			return Size::Dimen{ Size::TYPE::FIXED, jd.get<value_t>() };
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

		engine::graphics::data::Color color(const json & jdata) const
		{
			debug_assert(contains(jdata, "color"));
			return extract_color(jdata);
		}

		engine::graphics::data::Color color(const json & jdata, const engine::graphics::data::Color def_val) const
		{
			if (!contains(jdata, "color")) return def_val;
			return extract_color(jdata);
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

		std::string name(const json & jdata) const
		{
			debug_assert(contains(jdata, "name"));
			std::string str = jdata["name"];
			debug_assert(str.length() > std::size_t{ 0 });
			return str;
		}

		std::string name_or_null(const json & jdata) const
		{
			if (contains(jdata, "name"))
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

				margin.bottom = extract_margin("b", jmargin);
				margin.left = extract_margin("l", jmargin);
				margin.right = extract_margin("r", jmargin);
				margin.top = extract_margin("t", jmargin);
			}

			return margin;
		}

		bool has_res(const json & jdata) const
		{
			return contains(jdata, "res");
		}
		engine::Asset res(const json & jdata) const
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

		bool has_action(const json & jcomponent) { return contains(jcomponent, "trigger"); }

		void load_action(ViewData & view, const json & jcomponent)
		{
			const json & jtrigger = jcomponent["trigger"];

			engine::Asset action { jtrigger["action"].get<std::string>() };

			switch (action)
			{
			case ViewData::Action::CLOSE:
				view.action.type = ViewData::Action::CLOSE;
				break;
			case ViewData::Action::MOVER:
				view.action.type = ViewData::Action::MOVER;
				break;
			case ViewData::Action::SELECT:
				view.action.type = ViewData::Action::SELECT;
				break;

			default:
				debug_printline(0xffffffff, "GUI - unknown trigger action in component: ", jcomponent);
				debug_unreachable();
			}
		}

		bool has_function(const json & jcomponent) { return contains(jcomponent, "function"); }

		void load_function(ViewData & target, const json & jcomponent)
		{
			const json & jfunction = jcomponent["function"];

			target.function.name = this->load.name(jfunction);
			target.function.type = this->load.type(jfunction);

			const auto entity = engine::Entity::create();

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
				this->load.color(jtext, 0xff000000),
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
				this->load.res(jtexture));

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

		private:

			// checks if the view has a custom definition in the implementation of the view.
			bool is_customized(ViewData & data)
			{
				if (data.name.empty())
					return false;

				if (!contains(jdata, data.name))
				{
					// remove "template name" from instance if set and no overrider provided.
					// this is because the name needs to be unique.
					data.name = "";
					return false;
				}
				return true;
			}

			const json & get_customized(ViewData & data)
			{
				const json & jcustomize = this->jdata[data.name];

				// remove "template name" from instance if set and no overrider provided.
				// this is because the name needs to be unique.
				data.name = "";
				return jcustomize;
			}

		public:

			void operator() (GroupData & data)
			{
				if (is_customized(data))
				{
					const json & jcustomize = get_customized(data);

					data.name = this->load.name_or_null(jcustomize);
				}

				// customize any template children of the group
				for (auto & child : data.children)
				{
					visit(*this, child);
				}
			}
			void operator() (ListData & data)
			{
				if (is_customized(data))
				{
					const json & jcustomize = get_customized(data);

					data.name = this->load.name_or_null(jcustomize);
				}

				// customize the list item template
				for (auto & child : data.children)
				{
					visit(*this, child);
				}
			}
			void operator() (PanelData & data)
			{
				if (is_customized(data))
				{
					const json & jcustomize = get_customized(data);

					data.name = this->load.name_or_null(jcustomize);

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
				if (is_customized(data))
				{
					const json & jcustomize = get_customized(data);

					data.name = this->load.name_or_null(jcustomize);

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
				if (is_customized(data))
				{
					const json & jcustomize = get_customized(data);

					data.name = this->load.name_or_null(jcustomize);

					if (this->load.has_type_texture(jcustomize))
					{
						const json & jtexture = this->load.type_texture(jcustomize);

						if (this->load.has_res(jtexture))
							data.res = this->load.res(jtexture);
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
