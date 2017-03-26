
#ifndef GAMEPLAY_ASSETS_HPP
#define GAMEPLAY_ASSETS_HPP

#include <engine/common.hpp>

namespace gameplay
{
namespace asset
{
	struct asset_t
	{
		::engine::Entity id;
		::engine::transform_t transform;
	};

	struct droid_t : asset_t
	{

	};

	struct turret_t : asset_t
	{
		struct head_t
		{
			::engine::Entity id;
			::engine::Entity jointId;
			::engine::transform_t pivot;
		};

		struct barrel_t
		{
			::engine::Entity id;
			::engine::Entity jointId;
			::engine::transform_t pivot;
		};

		head_t head;

		barrel_t barrel;

		::engine::transform_t projectile;

		float timestamp;
	};
}
}

#endif // GAMEPLAY_ASSETS_HPP
