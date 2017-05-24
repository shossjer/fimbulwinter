
#ifndef ENGINE_MODEL_LOADER_HPP
#define ENGINE_MODEL_LOADER_HPP

#include <engine/common.hpp>

#include <string>
#include <unordered_map>

namespace engine
{
namespace model
{
	// the data needed when creating instances of an asset.
	// renderer meshes and physics shapes should already be added in respective module.
	struct asset_template_t
	{
		struct part_t
		{
			engine::Asset asset;
		};

		std::unordered_map<std::string, part_t> parts;

		struct joint_t
		{
			transform_t transform;
		};

		std::unordered_map<std::string, joint_t> joints;

		struct location_t
		{
			transform_t transform;
		};

		std::unordered_map<std::string, location_t> locations;

		template<typename T>
		const T & get(
			const std::unordered_map<std::string, T> & items,
			const std::string name) const
		{
			auto itr = items.find(name);

			if (itr == items.end())
			{
				debug_unreachable();
			}

			return itr->second;
		}

		const part_t & part(const std::string name) const
		{
			return get<part_t>(this->parts, name);
		}

		const joint_t & joint(const std::string name) const
		{
			return get<joint_t>(this->joints, name);
		}

		const location_t & location(const std::string name) const
		{
			return get<location_t>(this->locations, name);
		}
	};

	const asset_template_t & load(
			const engine::Asset asset,
			const std::string & modelName);
}
}

#endif // ENGINE_MODEL_LOADER_HPP
