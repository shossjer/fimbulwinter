#pragma once

#include "engine/Hash.hpp"
#include "engine/Token.hpp"
#include "engine/file/system/callbacks.hpp"
#include "engine/module.hpp"

#include "ful/heap.hpp"

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
		struct config_t;
		struct system;
		struct system_impl;

		class directory
		{
			friend struct system;

		private:
#if defined(_MSC_VER)
			ful::heap_string_utfw filepath_;
#else
			ful::heap_string_utf8 filepath_;
#endif

		private:
			directory() = default;

		public:
			// note not thread safe
			static directory working_directory();
		};

		struct system : module<system, system_impl>
		{
			using module<system, system_impl>::module;

			static system_impl * construct(engine::task::scheduler & taskscheduler, directory && root, config_t && config);
			static void destruct(system_impl & impl);
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
				REPORT_MISSING = uint32_t(1) << 5,
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

		constexpr const engine::Hash working_directory = engine::Hash("_working directory_");

		void register_directory(system & system, engine::Hash name, ful::heap_string_utf8 && filepath, engine::Hash parent);
		void register_temporary_directory(system & system, engine::Hash name);
		void unregister_directory(system & system, engine::Hash name);

		// mode ADD_WATCH | RECURSE_DIRECTORIES | REPORT_MISSING
		void read(
			system & system,
			engine::Token id,
			engine::Hash directory,
			ful::heap_string_utf8 && filepath,
			engine::Hash strand,
			read_callback * callback,
			utility::any && data,
			flags mode = flags{});

		void remove_watch(
			system & system,
			engine::Token id);

		// mode IGNORE_EXISTING | ADD_WATCH | RECURSE_DIRECTORIES
		void scan(
			system & system,
			engine::Token id,
			engine::Hash directory,
			engine::Hash strand,
			scan_callback * callback,
			utility::any && data,
			flags mode = flags{});

		// mode OVERWRITE_EXISTING | APPEND_EXISTING | CREATE_DIRECTORIES
		void write(
			system & system,
			engine::Hash directory,
			ful::heap_string_utf8 && filepath,
			engine::Hash strand,
			write_callback * callback,
			utility::any && data,
			flags mode = flags{});
	}
}
