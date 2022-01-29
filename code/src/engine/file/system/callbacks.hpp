#pragma once

#include "engine/Hash.hpp"

#include "ful/heapfwd.hpp"

namespace core
{
	class content;
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
			core::content & content,
			utility::any & data);

		using scan_callback = void(
			engine::file::system & filesystem,
			engine::Hash directory,
			ful::heap_string_utf8 && existing_files, // multiple files separated by ;
			ful::heap_string_utf8 && removed_files, // multiple files separated by ;
			utility::any & data);

		using write_callback = ext::ssize(
			engine::file::system & filesystem,
			core::content & content,
			utility::any && data);
	}
}
