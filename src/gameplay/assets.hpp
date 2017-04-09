
#ifndef GAMEPLAY_ASSETS_HPP
#define GAMEPLAY_ASSETS_HPP

#include <engine/common.hpp>

namespace gameplay
{
namespace asset
{
	using Entity = engine::Entity;

	struct asset_t
	{
		::engine::Entity id;
		::engine::transform_t transform;

		asset_t()
		{}

		asset_t(::engine::Entity id, ::engine::transform_t transform)
			:
			id(id),
			transform(transform)
		{
		}
	};

	struct droid_t : asset_t
	{
		::engine::Entity headId;

		::engine::Entity jointId;

		droid_t(Entity id, ::engine::transform_t transform, Entity headId, Entity jointId)
			:
			asset_t(id, transform),
			headId(headId),
			jointId(jointId)
		{
		}

		droid_t()
			:
			asset_t()
		{
		}
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
