#pragma once

#include "engine/Asset.hpp"

#include "engine/file/common.hpp"

// todo forward declare
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
	namespace task
	{
		struct scheduler;
	}
}

namespace engine
{
	namespace file
	{
		class system;

		class directory
		{
			friend class system;

		private:
#if defined(_MSC_VER)
			utility::heap_string_utfw filepath_;
#else
			utility::heap_string_utf8 filepath_;
#endif

		private:
			directory() = default;

			explicit directory(ext::usize size)
				: filepath_(size)
			{}

		public:
			// note not thread safe
			static directory working_directory();
		};

		class system
		{
		public:
			~system();
			explicit system(engine::task::scheduler & taskscheduler, directory && root);
		};

		struct flags
		{
			enum bits : uint32_t
			{
				OVERWRITE_EXISTING = uint32_t(1) << 0,
				APPEND_EXISTING = uint32_t(1) << 1,
				ADD_WATCH = uint32_t(1) << 2,
				CREATE_DIRECTORIES = uint32_t(1) << 3,
				RECURSE_DIRECTORIES = uint32_t(1) << 4,
			};

		private:
			uint32_t value_;

		public:
			flags() = default;
			constexpr flags(bits bit) : value_(bit) {}
		private:
			explicit constexpr flags(uint32_t value) : value_(value) {}

		public:
			explicit constexpr operator bool () const { return value_ != 0; }

			friend constexpr flags operator & (flags a, flags b) { return flags(a.value_ & b.value_); }
			friend constexpr flags operator | (flags a, flags b) { return flags(a.value_ | b.value_); }
			friend constexpr flags operator & (bits a, bits b) { return flags(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
			friend constexpr flags operator | (bits a, bits b) { return flags(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }

			friend constexpr bool operator == (flags a, flags b) { return a.value_ == b.value_; }
			friend constexpr bool operator != (flags a, flags b) { return !(a == b); }
		};

		constexpr const engine::Asset working_directory = engine::Asset{static_cast<uint32_t>(-1)};

		void register_directory(system & system, engine::Asset name, utility::heap_string_utf8 && filepath, engine::Asset parent);
		void register_temporary_directory(system & system, engine::Asset name);
		void unregister_directory(system & system, engine::Asset name);

		// mode ADD_WATCH | RECURSE_DIRECTORIES
		void read(
			system & system,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			engine::Asset strand,
			read_callback * callback,
			utility::any && data,
			flags mode = flags{});

		void remove_watch(
			system & system,
			engine::Asset directory,
			flags mode = flags{});
		void remove_watch(
			system & system,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			flags mode = flags{});

		using scan_callback = void(
			engine::Asset directory,
			utility::heap_string_utf8 && files, // multiple files separated by ;
			utility::any & data);

		// mode IGNORE_EXISTING | ADD_WATCH | RECURSE_DIRECTORIES
		void scan(
			system & system,
			engine::Asset directory,
			engine::Asset strand,
			scan_callback * callback,
			utility::any && data,
			flags mode = flags{});

		using write_callback = void(
			core::WriteStream && stream,
			utility::any && data);

		// mode OVERWRITE_EXISTING | APPEND_EXISTING | CREATE_DIRECTORIES
		void write(
			system & system,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			engine::Asset strand,
			write_callback * callback,
			utility::any && data,
			flags mode = flags{});
	}
}
