#pragma once

#include "engine/Asset.hpp"
#include "engine/module.hpp"

namespace core
{
	class ReadStream;
}

namespace engine
{
	namespace task
	{
		struct scheduler;
	}
}

namespace utility
{
	class any;
}

namespace engine
{
	namespace file
	{
		struct loader_impl;
		struct system;

		struct loader : module<loader, loader_impl>
		{
			using module<loader, loader_impl>::module;

			static loader_impl * construct(engine::task::scheduler & taskscheduler, engine::file::system & filesystem);
			static void destruct(loader_impl & impl);
		};

		using load_callback = void(
			core::ReadStream && stream,
			utility::any & data,
			engine::Asset file);
		using unload_callback = void(
			utility::any & data,
			engine::Asset file);

		void register_library(loader & loader, engine::Asset directory);
		void unregister_library(loader & loader, engine::Asset directory);

		void register_filetype(loader & loader, engine::Asset filetype, load_callback * loadcall, unload_callback * unloadcall, utility::any && data);
		void unregister_filetype(loader & loader, engine::Asset filetype);

		using ready_callback = void(
			utility::any & data,
			engine::Asset file);
		using unready_callback = void(
			utility::any & data,
			engine::Asset file);

		void load_global(
			loader & loader,
			engine::Asset filetype,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data);

		void load_local(
			loader & loader,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data);

		void load_dependency(
			loader & loader,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data);

		void unload_global(
			loader & loader,
			engine::Asset file);
		void unload_local(
			loader & loader,
			engine::Asset owner,
			engine::Asset file);
		void unload_dependency(
			loader & loader,
			engine::Asset owner,
			engine::Asset file);

		class scoped_filetype
		{
		private:

			engine::file::loader & loader_;
			engine::Asset filetype_;

		public:

			~scoped_filetype()
			{
				engine::file::unregister_filetype(loader_, filetype_);
			}

			explicit scoped_filetype(engine::file::loader & loader, engine::Asset filetype, load_callback * loadcall, unload_callback * unloadcall, utility::any && data)
				: loader_(loader)
				, filetype_(filetype)
			{
				engine::file::register_filetype(loader_, filetype_, loadcall, unloadcall, std::move(data));
			}

		public:

			operator engine::Asset () const { return filetype_; }
		};
	}
}
