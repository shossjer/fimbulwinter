#pragma once

#include "engine/Hash.hpp"

// todo forward declare string
#include "utility/unicode/string.hpp"

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
		struct system;

		using read_callback = void(
			engine::file::system & filesystem,
			core::ReadStream && stream,
			utility::any & data);

		using scan_callback = void(
			engine::file::system & filesystem,
			engine::Hash directory,
			utility::heap_string_utf8 && existing_files, // multiple files separated by ;
			utility::heap_string_utf8 && removed_files, // multiple files separated by ;
			utility::any & data);

		using write_callback = void(
			engine::file::system & filesystem,
			core::WriteStream && stream,
			utility::any && data);
	}
}
