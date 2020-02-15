
#ifndef CORE_JSONSERIALIZER_HPP
#define CORE_JSONSERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/WriteStream.hpp"

#include "utility/json.hpp"

#include <cstdint>

namespace core
{
	class JsonSerializer
	{
	private:
		core::WriteStream stream;

	public:
		JsonSerializer(core::WriteStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void write(const T & x)
		{
			json root;
			write_object(root, x);

			const std::string dump = root.dump(4) + "\n";
			debug_verify(stream.write_all(dump.data(), dump.size()) == dump.size());
		}

	private:
		void write_(json & j, const bool & x)
		{
			write_bool(j, x);
		}
		void write_(json & j, const int & x)
		{
			write_number(j, x);
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_enum<T>::value))>
		void write_(json & j, const T & x)
		{
			write_string(j, core::value_table<T>::get_key(x));
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_class<T>::value))>
		void write_(json & j, const T & x)
		{
			write_object(j, x);
		}

		void write_bool(json & j, const bool & x)
		{
			j = x;
		}

		void write_number(json & j, const int & x)
		{
			j = x;
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value))>
		void write_object(json & j, const T & x)
		{
			member_table<T>::for_each_member(x, [&](const auto& name, const auto& y){ write_(j[std::string(name.begin(), name.end())], y); } );
		}

		void write_string(json & j, const utility::string_view & x)
		{
			j = std::string(x.begin(), x.end());
		}
	};
}

#endif /* CORE_JSONSERIALIZER_HPP */
