#pragma once

#include "engine/Asset.hpp"

namespace core
{
	class ReadStream;
}

namespace utility
{
	class any;
}

namespace engine
{
	namespace file
	{
		using read_callback = void(
			core::ReadStream && stream,
			utility::any & data);
	}
}
