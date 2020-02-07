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

		void register_directory(engine::Asset debug_expression(name), utility::heap_string_utf8 && debug_expression(path))
		{
			debug_printline("todo register_directory(", name, ", ", path, ")");
		}

		void register_temporary_directory(engine::Asset debug_expression(name))
		{
			debug_printline("todo register_temporary_directory(", name, ")");
		}

		void unregister_directory(engine::Asset debug_expression(name))
		{
			debug_printline("todo unregister_directory(", name, ")");
		}

		void read(
			engine::Asset debug_expression(directory),
			utility::heap_string_utf8 && debug_expression(pattern),
			watch_callback * debug_expression(callback),
			utility::any && debug_expression(data))
		{
			debug_printline("todo read(", directory, ", ", pattern, ", ", callback, ", ", data, ")");
		}

		void watch(
			engine::Asset debug_expression(directory),
			utility::heap_string_utf8 && debug_expression(pattern),
			watch_callback * debug_expression(callback),
			utility::any && debug_expression(data))
		{
			debug_printline("todo watch(", directory, ", ", pattern, ", ", callback, ", ", data, ")");
		}

		void write(
			engine::Asset debug_expression(directory),
			utility::heap_string_utf8 && debug_expression(filename),
			write_callback * debug_expression(callback),
			utility::any && debug_expression(data))
		{
			debug_printline("todo write(", directory, ", ", filename, ", ", callback, ", ", data, ")");
		}
	}
}

#endif
