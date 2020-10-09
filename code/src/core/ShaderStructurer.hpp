#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/StringStream.hpp"

#include "utility/regex.hpp"
#include "utility/unicode/string_view.hpp"

namespace core
{
	class ShaderStructurer
	{
	private:

		StringStream<utility::heap_storage_traits, utility::encoding_utf8> stream_;

		using Parser = rex::parser_for<StringStream<utility::heap_storage_traits, utility::encoding_utf8>>;

	public:

		ShaderStructurer(ReadStream && stream)
			: stream_(std::move(stream))
		{}

	public:

		template <typename T>
		void read(T & x)
		{
			Parser parser = rex::parse(stream_);

			const auto vertex_section = parser.match(rex::str("[vertex]") >> rex::newline);
			if (!debug_verify(vertex_section.first, stream_.filepath(), ":", vertex_section.second - stream_.begin(), ": expected vertex section and newline"))
				return; // error

			parser.seek(vertex_section.second);

			using core::clear;
			clear<core::member_table<T>::find("inputs")>(x);
			while (true)
			{
				const auto property = parser.match(rex::ch('['));
				if (!property.first)
					break;

				parser.seek(property.second);

				const auto bind = parser.match(rex::str("bind") >> rex::blank);
				if (!bind.first)
					return; // error

				parser.seek(bind.second);

				const auto bind_name_value = read_name_value(x, mpl::index_constant<core::member_table<T>::find("inputs")>{}, parser);
				if (!bind_name_value.first)
					return; // error

				parser.seek(bind_name_value.second);

				const auto bind_name_value_end = parser.match(rex::str("]") >> rex::newline);
				if (!debug_verify(bind_name_value_end.first, stream_.filepath(), ":", vertex_section.second - stream_.begin(), ": expected ending bracket and newline"))
					return; // error

				parser.seek(bind_name_value_end.second);
			}

			const auto fragment_section = parser.find(rex::str("[fragment]") >> rex::newline); // todo beginline
			if (!debug_verify(fragment_section.first != fragment_section.second, stream_.filepath(), ":?", ": expected fragment section and newline"))
				return; // error

			using core::serialize;
			debug_verify(serialize<core::member_table<T>::find("vertex_source")>(x, utility::string_units_utf8(parser.begin().get(), fragment_section.first.get())));

			parser.seek(fragment_section.second);

			using core::clear;
			clear<core::member_table<T>::find("outputs")>(x);
			while (true)
			{
				const auto bind = parser.match(rex::str("[bind") >> rex::blank);
				if (!bind.first)
					break;

				parser.seek(bind.second);

				const auto bind_name_value = read_name_value(x, mpl::index_constant<core::member_table<T>::find("outputs")>{}, parser);
				if (!bind_name_value.first)
					return; // error

				parser.seek(bind_name_value.second);

				const auto bind_name_value_end = parser.match(rex::str("]") >> rex::newline);
				if (!debug_verify(bind_name_value_end.first, stream_.filepath(), ":", vertex_section.second - stream_.begin(), ": expected ending bracket and newline"))
					return; // error

				parser.seek(bind_name_value_end.second);
			}

			const auto end = parser.find(rex::end);

			using core::serialize;
			debug_verify(serialize<core::member_table<T>::find("fragment_source")>(x, utility::string_units_utf8(parser.begin().get(), end.first.get())));
		}

	private:

		template <typename T>
		auto read_name_value(T & /*x*/, mpl::index_constant<std::size_t(-1)>, Parser parser)
		{
			const auto nextline = parser.find(rex::str("]"));
			return std::make_pair(true, nextline.first);
		}

		template <typename T, std::size_t I>
		auto read_name_value(T & x, mpl::index_constant<I>, Parser parser)
		{
			using core::grow;
			auto & y = grow(core::member_table<T>::template get<I>(x));

			using core::serialize;

			const auto name = parser.match(+rex::word);
			if (!name.first)
				return name;

			debug_verify(serialize<core::member_table<mpl::remove_cvref_t<decltype(y)>>::find("name")>(y, utility::string_units_utf8(parser.begin().get(), name.second.get())));

			parser.seek(name.second);

			const auto space = parser.match(rex::blank);
			if (!space.first)
				return space;

			parser.seek(space.second);

			const auto value = parser.match(+rex::digit);
			if (!value.first)
				return value;

			debug_verify(serialize<core::member_table<mpl::remove_cvref_t<decltype(y)>>::find("value")>(y, utility::string_units_utf8(parser.begin().get(), value.second.get())));

			return value;
		}
	};
}
