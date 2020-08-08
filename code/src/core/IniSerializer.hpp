#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/WriteStream.hpp"

#include "utility/string.hpp"

#include <string>
#include <cstdint>

namespace core
{
	class IniSerializer
	{
	private:
		core::WriteStream stream;

	public:
		IniSerializer(core::WriteStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void write(const T & x)
		{
			write_key_values(x);
			write_headers(x);
		}
	private:
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_class<T>::value))>
		void write_key_value(const utility::string_units_utf8 &, const T &)
		{
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_enum<T>::value))>
		void write_key_value(const utility::string_units_utf8 & name, const T & x)
		{
			debug_verify(stream.write_all(name.data(), name.size()) == name.size());
			debug_verify(stream.write_all("=", sizeof "=" - 1) == sizeof "=" - 1);

			utility::string_units_utf8 value = value_table<T>::get_key(x);
			debug_verify(stream.write_all(value.data(), value.size()) == value.size());

			debug_verify(stream.write_all("\n", sizeof "\n" - 1) == sizeof "\n" - 1);
		}
		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		void write_key_value(const utility::string_units_utf8 & name, const T & x)
		{
			debug_verify(stream.write_all(name.data(), name.size()) == name.size());
			debug_verify(stream.write_all("=", sizeof "=" - 1) == sizeof "=" - 1);

			std::string value = utility::to_string(x);
			debug_verify(stream.write_all(value.data(), value.size()) == value.size());

			debug_verify(stream.write_all("\n", sizeof "\n" - 1) == sizeof "\n" - 1);
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
		void write_header(const utility::string_units_utf8 & name, const T & x)
		{
			debug_verify(stream.write_all("[", sizeof "[" - 1) == sizeof "[" - 1);
			debug_verify(stream.write_all(name.data(), name.size()) == name.size());
			debug_verify(stream.write_all("]", sizeof "]" - 1) == sizeof "]" - 1);
			debug_verify(stream.write_all("\n", sizeof "\n" - 1) == sizeof "\n" - 1);

			member_table<T>::for_each_member(x, [&](const auto& k, const auto& y){ write_key_value(k, y); } );

			debug_verify(stream.write_all("\n", sizeof "\n" - 1) == sizeof "\n" - 1);
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((!std::is_class<T>::value))>
		void write_header(const utility::string_units_utf8 &, const T &)
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
