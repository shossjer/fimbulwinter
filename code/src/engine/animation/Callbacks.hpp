#pragma once

#include "engine/Token.hpp"

namespace engine
{
namespace animation
{
	// todo remove
	class Callbacks
	{
	public:
		/**
		 *
		 */
		virtual void onFinish(const engine::Token id) const = 0;
	};
}
}
