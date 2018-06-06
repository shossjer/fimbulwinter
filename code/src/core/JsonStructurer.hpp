
#ifndef CORE_JSONSTRUCTURER_HPP
#define CORE_JSONSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/json.hpp"
#include "utility/optional.hpp"

#include <cstdint>
#include <vector>

namespace core
{
	class JsonStructurer
	{
	private:
		json root;

		std::string filename;

	public:
		JsonStructurer(std::string filename)
			: filename(std::move(filename))
		{}

	public:
		const json & get() const { return root; }

		template <typename T>
		void read(T & x)
		{
			read_object(root, x);
		}

		void set(const char * data, size_t size)
		{
			try
			{
				root = json::parse(data, data + size);
			}
			catch (...)
			{
				debug_fail("something is wrong with the json in '", filename, "'");
			}
		}
	private:
		template <typename T>
		void read_array(const json & j, std::vector<T> & x)
		{
			debug_assert(j.is_array());
			x.reserve(j.size());
			for (const json & v : j)
			{
				x.emplace_back();
				if (v.is_array())
				{
					read_array(v, x.back());
				}
				else if (v.is_number())
				{
					read_number(v, x.back());
				}
				else if (v.is_object())
				{
					read_object(v, x.back());
				}
				else if (v.is_string())
				{
					read_string(v, x.back());
				}
				else
				{
					debug_fail("unknown value type in json '", filename, "'");
				}
			}
		}
		template <typename T>
		void read_array(const json &, T &)
		{
			debug_fail("attempting to read array into a non array type in json '", filename, "'");
		}

		void read_number(const json & j, int & x)
		{
			x = j;
		}
		void read_number(const json & j, double & x)
		{
			x = j;
		}
		template <typename T>
		void read_number(const json & j, utility::optional<T> & x)
		{
			if (x.has_value())
			{
				read_number(j, x.value());
			}
			else
			{
				read_number(j, x.emplace());
			}
		}
		template <typename T>
		void read_number(const json &, T &)
		{
			debug_fail("attempting to read number into a non number type in json '", filename, "'");
		}

		template <typename T,
		          REQUIRES((core::has_member_table<T>::value))>
		void read_object(const json & j, T & x)
		{
			debug_assert(j.is_object());
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				const auto key_string = it.key();
				const utility::string_view key = key_string.c_str();
				if (!serialization<T>::has(key))
					continue;

				const json & v = it.value();
				if (v.is_array())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_array(v, y); });
				}
				else if (v.is_number())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_number(v, y); });
				}
				else if (v.is_object())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_object(v, y); });
				}
				else if (v.is_string())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_string(v, y); });
				}
				else
				{
					debug_fail("unknown value type in json '", filename, "'");
				}
			}
		}
		template <typename T,
		          REQUIRES((!core::has_member_table<T>::value))>
		void read_object(const json &, T &)
		{
			debug_fail("attempting to read object into a non class type (or a class type without a member table) in json '", filename, "'");
		}

		void read_string(const json & j, std::string & x)
		{
			x = j;
		}
		template <typename T>
		void read_string(const json &, T &)
		{
			debug_fail("attempting to read string into a non string type in json '", filename, "'");
		}
	};
}

#endif /* CORE_JSONSTRUCTURER_HPP */
