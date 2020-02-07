#include "config.h"

#if FILE_USE_DUMMY

#include "system.hpp"

#include "core/debug.hpp"

#include "utility/any.hpp"

namespace engine
{
	namespace file
	{
		system::~system()
		{}

		system::system()
		{}

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path)
		{
			debug_printline("todo register_directory(", name, ", ", path, ")");
		}

		void register_temporary_directory(engine::Asset name)
		{
			debug_printline("todo register_temporary_directory(", name, ")");
		}

		void unregister_directory(engine::Asset name)
		{
			debug_printline("todo unregister_directory(", name, ")");
		}

		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data)
		{
			debug_printline("todo read(", directory, ", ", pattern, ", ", callback, ", ", data, ")");
		}

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data)
		{
			debug_printline("todo watch(", directory, ", ", pattern, ", ", callback, ", ", data, ")");
		}

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data)
		{
			debug_printline("todo write(", directory, ", ", filename, ", ", callback, ", ", data, ")");
		}
	}
}

#endif
