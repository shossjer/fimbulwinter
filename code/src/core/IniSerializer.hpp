
#ifndef CORE_INISERIALIZER_HPP
#define CORE_INISERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/WriteStream.hpp"

#include "utility/string.hpp"
#include "utility/string_view.hpp"

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
		void write_key_value(const utility::string_view &, const T &)
		{
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_enum<T>::value))>
		void write_key_value(const utility::string_view & name, const T & x)
		{
			stream.write(name.data(), name.size());
			stream.write("=", sizeof "=" - 1);

			utility::string_view value = value_table<T>::get_key(x);
			stream.write(value.data(), value.size());

			stream.write("\n", sizeof "\n" - 1);
		}
		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		void write_key_value(const utility::string_view & name, const T & x)
		{
			stream.write(name.data(), name.size());
			stream.write("=", sizeof "=" - 1);

			std::string value = utility::to_string(x);
			stream.write(value.data(), value.size());

			stream.write("\n", sizeof "\n" - 1);
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
			stream.write("[", sizeof "[" - 1);
			stream.write(name.data(), name.size());
			stream.write("]", sizeof "]" - 1);
			stream.write("\n", sizeof "\n" - 1);

			member_table<T>::for_each_member(x, [&](const auto& k, const auto& y){ write_key_value(k, y); } );

			stream.write("\n", sizeof "\n" - 1);
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
