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

		struct flags
		{
			enum bits : uint32_t
			{
				REPORT_MISSING = uint32_t(1) << 0,
				IGNORE_EXISTING = uint32_t(1) << 1,
			};

		private:
			uint32_t value;

		public:
			flags() = default;
			constexpr flags(bits bit) : value(bit) {}
		private:
			explicit constexpr flags(uint32_t value) : value(value) {}

		public:
			explicit constexpr operator bool () const { return value != 0; }

			friend constexpr flags operator & (flags a, flags b) { return flags(a.value & b.value); }
		};

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path);
		void register_temporary_directory(engine::Asset name);
		void unregister_directory(engine::Asset name);

		using watch_callback = void(
			core::ReadStream && stream,
			utility::any & data,
			engine::Asset match);

		// mode REPORT_MISSING
		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode = flags{});

		void remove(engine::Asset directory, utility::heap_string_utf8 && pattern);

		// mode REPORT_MISSING | IGNORE_EXISTING
		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode = flags{});

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
