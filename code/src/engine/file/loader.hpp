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

		using loading_callback = void(
			core::ReadStream && stream,
			utility::any & data,
			engine::Asset file);
		using loaded_callback = void(
			utility::any & data,
			engine::Asset file);

		using unload_callback = void(
			utility::any & data,
			engine::Asset file);

		void register_library(loader & loader, engine::Asset directory);
		void unregister_library(loader & loader, engine::Asset directory);

		void load_global(
			loader & loader,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data);

		void load_local(
			loader & loader,
			engine::Asset owner,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data);

		void load_dependency(
			loader & loader,
			engine::Asset owner,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data);

		void unload_global(loader & loader, engine::Asset file);
		void unload_local(loader & loader, engine::Asset owner, engine::Asset file);
		void unload_dependency(loader & loader, engine::Asset owner, engine::Asset file);
	}
}
