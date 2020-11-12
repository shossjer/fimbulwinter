#pragma once

#include "engine/file/common.hpp"

namespace engine
{
	namespace file
	{
		class system;

		class loader
		{
		public:
			~loader();
			loader(system & system);
		};

		using load_callback = void(
			core::ReadStream && stream,
			utility::any & data,
			engine::Asset file);

		using purge_callback = void(
			utility::any && data,
			engine::Asset match);

		void register_library(loader & loader, engine::Asset directory);
		void unregister_library(loader & loader, engine::Asset directory);

		void load_global(
			loader & loader,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data);

		void load_local(
			loader & loader,
			engine::Asset owner,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data);

		void load_dependency(
			loader & loader,
			engine::Asset owner,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data);

		void unload_global(loader & loader, engine::Asset file);
		void unload_local(loader & loader, engine::Asset owner, engine::Asset file);
		void unload_dependency(loader & loader, engine::Asset owner, engine::Asset file);
	}
}
