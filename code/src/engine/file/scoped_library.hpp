#pragma once

#include "engine/file/loader.hpp"

namespace engine
{
	namespace file
	{
		class scoped_library
		{
		private:

			loader & loader_;
			engine::Hash directory_;

		public:

			~scoped_library()
			{
				unregister_library(loader_, directory_);
			}

			explicit scoped_library(loader & loader, engine::Hash directory)
				: loader_(loader)
				, directory_(directory)
			{
				register_library(loader_, directory_);
			}
		};
	}
}
