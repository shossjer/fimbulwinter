
#include "exception.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"

#include "utility/json.hpp"

namespace engine
{
	namespace gui
	{
		using key_t = std::string;

		class ResourceLoader
		{
		public:

			const json & jdimensions;
			const json & jstrings;

		public:

			ResourceLoader(json & jcontent)
				: jdimensions(find_or_create(jcontent, "dimensions"))
				, jstrings(find_or_create(jcontent, "strings"))
			{
			}

		public:

			auto string(const key_t key, const json & jdata) const
			{
				if (!contains(jdata, key))
				{
					debug_printline(engine::gui_channel, "WARNING - key: ", key, "; missing in data: ", jdata);
					throw key_missing(key);
				}

				return parse_as_string(jdata[key]);
			}
			auto string(const key_t key, const json & jdata, std::string && def) const
			{
				if (!contains(jdata, key))
					return def;

				return parse_as_string(jdata[key]);
			}
			auto string_or_empty(const key_t key, const json & jdata) const
			{
				if (!contains(jdata, key))
				{
					return std::string{};
				}

				return parse_as_string(jdata[key]);
			}

			template<typename N>
			N number(const key_t key, const json & jdata) const
			{
				if (!contains(jdata, key))
				{
					debug_printline(engine::gui_channel, "WARNING - key: ", key, "; missing in data: ", jdata);
					throw key_missing(key);
				}
				return parse_as_number<N>(jdata[key]);
			}
			template<typename N>
			N number(const key_t key, const json & jdata, const N def) const
			{
				if (!contains(jdata, key))
					return def;

				return parse_as_number<N>(jdata[key]);
			}

		private:

			std::string parse_as_string(const json & jv) const
			{
				auto str = jv.get<std::string>();

				if (!str.empty() && str[0] == '#')
				{
					return resource_string(str);
				}
				return str;
			}

			template<typename N>
			N parse_as_number(const json & jv) const
			{
				if (jv.is_number())
				{
					return jv.get<N>();
				}

				if (jv.is_string())
				{
					const std::string str = jv;
					if (str[0] == '#')
					{
						return this->jdimensions[str].get<N>();
					}
					debug_printline(engine::gui_channel, "WARNING: cannot load number alias: ", jv);
				}
				else
				{
					debug_printline(engine::gui_channel, "WARNING: cannot load number type: ", jv);
				}
				throw bad_json("Invalid number");
			}

			std::string resource_string(const key_t & key) const
			{
				if (contains(jstrings, key))
				{
					return jstrings[key].get<std::string>();
				}

				throw key_missing(key);
			}

			template<typename N>
			N resource_number(const key_t & key) const
			{
				if (contains(jdimensions, key))
				{
					return jdimensions[key].get<N>();
				}

				throw key_missing(key);
			}

			static const json & find_or_create(json & jcontent, const key_t & key)
			{
				if (contains(jcontent, key))
				{
					return jcontent[key];
				}

				jcontent[key] = json::object();
				return jcontent[key];
			}
		};
	}
}
