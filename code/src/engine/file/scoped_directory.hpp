#pragma once

#include "engine/file/system.hpp"

namespace engine
{
	namespace file
	{
		class scoped_directory
		{
		private:

			system & system_;
			engine::Hash name_;

		public:

			~scoped_directory()
			{
				unregister_directory(system_, name_);
			}

			explicit scoped_directory(system & system, engine::Hash name)
				: system_(system)
				, name_(name)
			{
				register_temporary_directory(system_, name_);
			}

			explicit scoped_directory(system & system, engine::Hash name, ful::heap_string_utf8 && filepath, engine::Hash parent)
				: system_(system)
				, name_(name)
			{
				register_directory(system_, name_, std::move(filepath), parent);
			}

			operator engine::Hash() const { return name_; }
		};
	}
}
