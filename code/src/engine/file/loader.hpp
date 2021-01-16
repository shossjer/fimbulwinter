#pragma once

#include "engine/Asset.hpp"
#include "engine/module.hpp"
#include "engine/Token.hpp"

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
			loader & loader,
			core::ReadStream && stream,
			utility::any & stash,
			engine::Asset file);
		using unload_callback = void(
			loader & loader,
			utility::any & stash,
			engine::Asset file);

		void register_library(loader & loader, engine::Hash directory);
		void unregister_library(loader & loader, engine::Hash directory);

		void register_filetype(loader & loader, engine::Hash filetype, load_callback * loadcall, unload_callback * unloadcall);
		void unregister_filetype(loader & loader, engine::Hash filetype);

		using ready_callback = void(
			loader & loader,
			utility::any & data,
			engine::Asset name,
			const utility::any & stash,
			engine::Asset file);
		using unready_callback = void(
			loader & loader,
			utility::any & data,
			engine::Asset name,
			const utility::any & stash,
			engine::Asset file);

		void load_independent(
			loader & loader,
			engine::Token tag,
			engine::Asset name,
			engine::Hash filetype,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data);

		void load_dependency(
			loader & loader,
			engine::Asset owner,
			engine::Asset name,
			engine::Hash filetype,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data);

		void unload_independent(
			loader & loader,
			engine::Token tag);

		class scoped_filetype
		{
		private:

			engine::file::loader & loader_;
			engine::Hash filetype_;

		public:

			~scoped_filetype()
			{
				engine::file::unregister_filetype(loader_, filetype_);
			}

			explicit scoped_filetype(engine::file::loader & loader, engine::Hash filetype, load_callback * loadcall, unload_callback * unloadcall)
				: loader_(loader)
				, filetype_(filetype)
			{
				engine::file::register_filetype(loader_, filetype_, loadcall, unloadcall);
			}

		public:

			operator engine::Hash() const { return filetype_; }
		};
	}
}
