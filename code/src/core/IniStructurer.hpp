#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/StringStream.hpp"

#include "utility/regex.hpp"

namespace core
{
	class IniStructurer
	{
	private:

		StringStream<utility::heap_storage_traits> stream_;

		using Parser = rex::parser_for<StringStream<utility::heap_storage_traits>>;

	public:

		explicit IniStructurer(ReadStream && stream)
			: stream_(std::move(stream))
		{}

	public:

		template <typename T>
		bool read(T & x)
		{
			Parser parser = rex::parse(stream_);

			const auto key_values = read_key_values(x, parser);
			if (!key_values.first)
				return false; // error

			parser.seek(key_values.second);

			const auto headers = read_headers(x, parser);
			if (!headers.first)
				return false; // error

			return debug_verify(headers.second == parser.end());
		}

	private:

		auto skip_key_value(Parser parser)
		{
			const auto key_match = parser.match(+(!rex::ch('=') - rex::newline));
			if (!debug_verify(key_match.first))
				return key_match;

			parser.seek(key_match.second);

			const auto equals_match = parser.match(rex::ch('='));
			if (!debug_verify(equals_match.first, equals_match.second - stream_.begin()))
				return equals_match;

			parser.seek(equals_match.second);

			const auto value_match = parser.match(*!rex::newline);
			if (!debug_verify(value_match.first))
				return value_match;

			return value_match;
		}

		auto skip_key_values(Parser parser)
		{
			do
			{
				parser.seek(parser.match(*(-(rex::ch(';') >> *!rex::newline) >> rex::newline)).second);
				if (ext::empty(parser))
					break;

				const auto maybe_header = parser.match(rex::ch('['));
				if (maybe_header.first)
					break;

				const auto key_value = skip_key_value(parser);
				if (!key_value.first)
					return key_value;

				parser.seek(key_value.second);
			}
			while (!ext::empty(parser));

			return std::make_pair(true, parser.begin());
		}

		template <typename T>
		auto read_header(T & x, Parser parser)
		{
			const auto bracket_open = parser.match(rex::ch('['));
			if (!debug_verify(bracket_open.first))
				return bracket_open;

			parser.seek(bracket_open.second);

			const auto bracket_close = parser.find(rex::ch(']') | rex::newline);
			if (!debug_verify(bracket_close.first != parser.end()))
				return std::make_pair(false, bracket_close.first);

			const auto header_name = ful::view_utf8(bracket_open.second.get(), bracket_close.first.get());

			parser.seek(bracket_close.first);

			if (!debug_verify(parser.match(rex::ch(']')).first))
				return std::make_pair(false, bracket_close.first);

			parser.seek(bracket_close.second);

			const auto member = core::member_table<T>::find(header_name);
			if (member != std::size_t(-1))
			{
				return core::member_table<T>::call(member, x, [&](auto & y){ return read_key_values(y, parser); });
			}
			else
			{
				return skip_key_values(parser);
			}
		}

		template <typename T>
		auto read_headers(T & x, Parser parser)
		{
			do
			{
				parser.seek(parser.match(*(-(rex::ch(';') >> *!rex::newline) >> rex::newline)).second);
				if (ext::empty(parser))
					break;

				const auto header = read_header(x, parser);
				if (!header.first)
					return header;

				parser.seek(header.second);
			}
			while (!ext::empty(parser));

			return std::make_pair(true, parser.begin());
		}

		template <typename T>
		auto read_key_value(T & x, Parser parser)
		{
			const auto key_match = parser.match(+(!rex::ch('=') - rex::newline));
			if (!debug_verify(key_match.first))
				return key_match;

			const auto key = ful::view_utf8(parser.begin().get(), key_match.second.get());

			parser.seek(key_match.second);

			const auto equals_match = parser.match(rex::ch('='));
			if (!debug_verify(equals_match.first))
				return equals_match;

			parser.seek(equals_match.second);

			const auto value_match = parser.match(*!rex::newline);
			if (!debug_verify(value_match.first))
				return value_match;

			const auto value = ful::view_utf8(parser.begin().get(), value_match.second.get());

			const auto member = core::member_table<T>::find(key);
			if (member != std::size_t(-1))
			{
				if (!core::member_table<T>::call(member, x,
				                                 [&](auto & y)
				                                 {
					                                 // todo read directly from parser
					                                 return static_cast<bool>(fio::istream<ful::view_utf8>(value) >> y);
				                                 }))
					return std::make_pair(false, equals_match.second);
			}
			return value_match;
		}

		template <typename T,
		          REQUIRES((std::is_class<T>::value))>
		auto read_key_values(T & x, Parser parser)
		{
			do
			{
				parser.seek(parser.match(*(-(rex::ch(';') >> *!rex::newline) >> rex::newline)).second);
				if (ext::empty(parser))
					break;

				const auto maybe_header = parser.match(rex::ch('['));
				if (maybe_header.first)
					break;

				const auto key_value = read_key_value(x, parser);
				if (!key_value.first)
					return key_value;

				parser.seek(key_value.second);
			}
			while (!ext::empty(parser));

			return std::make_pair(true, parser.begin());
		}

		template <typename T,
		          REQUIRES((!std::is_class<T>::value))>
		auto read_key_values(T &, Parser parser)
		{
			constexpr auto type_name = utility::type_name<T>();
			static_cast<void>(type_name);

			return std::make_pair(debug_fail("cannot read key values into object of type '", type_name, "'"), parser.begin());
		}
	};
}
