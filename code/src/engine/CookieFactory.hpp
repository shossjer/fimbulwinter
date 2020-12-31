#pragma once

#include "engine/Cookie.hpp"

#include <atomic>

namespace engine
{
	class CookieFactory
	{
	private:

		std::atomic<Cookie::value_type> next_id_;

	public:

		CookieFactory()
			: next_id_{1} // reserve 0
		{}

	public:

		Cookie create()
		{
			const Cookie::value_type id = next_id_.fetch_add(1, std::memory_order_relaxed);

			return Cookie{id};
		}
	};
}
