#pragma once

#include "engine/Asset.hpp"

// todo forward declare
#include "utility/unicode.hpp"

namespace core
{
	class ReadStream;
	class WriteStream;
}

namespace utility
{
	class any;
}

namespace engine
{
	namespace file
	{
		struct system
		{
			~system();
			system();
		};

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path);
		void register_temporary_directory(engine::Asset name);
		void unregister_directory(engine::Asset name);

		using watch_callback = void(
			core::ReadStream && stream,
			utility::any & data,
			engine::Asset match);

		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data);

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data);

		using write_callback = void(
			core::WriteStream && stream,
			utility::any && data);

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data);
	}
}
