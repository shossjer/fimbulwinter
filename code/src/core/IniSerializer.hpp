
#ifndef CORE_INISERIALIZER_HPP
#define CORE_INISERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/string.hpp"

#include <string>
#include <vector>
#include <cstdint>

namespace core
{
	class IniSerializer
	{
	private:
		std::vector<char> buffer;

		std::string filename;

	public:
		IniSerializer(std::string filename)
			: filename(std::move(filename))
		{}

	public:
		const char * data() const { return buffer.data(); }
		std::size_t size() const { return buffer.size(); }

		template <typename T>
		void write(const T & x)
		{
			buffer.clear();

			write_key_values(x);
			write_headers(x);
		}
	private:
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_class<T>::value))>
		void write_key_value(const utility::string_view &, const T &)
		{
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_enum<T>::value))>
		void write_key_value(const utility::string_view & name, const T & x)
		{
			buffer.insert(buffer.end(), name.begin(), name.end());
			buffer.push_back('=');

			utility::string_view value = value_table<T>::get_key(x);
			buffer.insert(buffer.end(), value.begin(), value.end());

			buffer.push_back('\n');
		}
		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		void write_key_value(const utility::string_view & name, const T & x)
		{
			buffer.insert(buffer.end(), name.begin(), name.end());
			buffer.push_back('=');

			std::string value = utility::to_string(x);
			buffer.insert(buffer.end(), value.begin(), value.end());

			buffer.push_back('\n');
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value))>
		void write_key_values(const T & x)
		{
			member_table<T>::for_each_member(x, [&](const auto& name, const auto& y){ write_key_value(name, y); } );
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_class<T>::value))>
		void write_header(const utility::string_view & name, const T & x)
		{
			buffer.push_back('[');
			buffer.insert(buffer.end(), name.begin(), name.end());
			buffer.push_back(']');
			buffer.push_back('\n');

			member_table<T>::for_each_member(x, [&](const auto& k, const auto& y){ write_key_value(k, y); } );

			buffer.push_back('\n');
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((!std::is_class<T>::value))>
		void write_header(const utility::string_view &, const T &)
		{
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value))>
		void write_headers(const T & x)
		{
			member_table<T>::for_each_member(x, [&](const auto& name, const auto& y){ write_header(name, y); } );
		}
	};
}

#endif /* CORE_INISERIALIZER_HPP */
