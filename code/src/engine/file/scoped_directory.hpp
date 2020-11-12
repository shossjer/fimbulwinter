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
			engine::Asset name_;

		public:

			~scoped_directory()
			{
				unregister_directory(system_, name_);
			}

			explicit scoped_directory(system & system, engine::Asset name)
				: system_(system)
				, name_(name)
			{
				register_temporary_directory(system_, name_);
			}

			explicit scoped_directory(system & system, engine::Asset name, utility::heap_string_utf8 && filepath, engine::Asset parent)
				: system_(system)
				, name_(name)
			{
				register_directory(system_, name_, std::move(filepath), parent);
			}

			operator engine::Asset() const { return name_; }
		};
	}
}
