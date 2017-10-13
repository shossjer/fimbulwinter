
#ifndef ENGINE_EXTERNAL_LOADER_HPP
#define ENGINE_EXTERNAL_LOADER_HPP

#include <core/container/Buffer.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Asset.hpp>
#include <engine/common.hpp>
#include <engine/Entity.hpp>
#include <engine/graphics/renderer.hpp>

#include <utility/variant.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace engine
{
	namespace resource
	{
		namespace loader
		{
			struct asset_template_t
			{
				struct component_t
				{
					engine::Asset mesh;
					engine::graphics::data::Color color;
				};

				std::unordered_map<
					std::string,
					std::vector<component_t> > components;

				struct joint_t
				{
					engine::transform_t transform;
				};

				std::unordered_map<std::string, joint_t> joints;

				struct location_t
				{
					engine::transform_t transform;
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

				const std::vector<component_t> & part(const std::string name) const
				{
					return get<std::vector<component_t> >(this->components, name);
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

			struct Level
			{
				struct Mesh
				{
					std::string name;
					core::maths::Matrix4x4f matrix;
					unsigned int color;
				};
				struct Placeholder
				{
					std::string name;
					core::maths::Vector3f translation;
					core::maths::Quaternionf rotation;
					core::maths::Vector3f scale;
				};

				std::vector<Mesh> meshes;
				std::vector<Placeholder> placeholders;
			};

			struct Placeholder
			{
				utility::variant<int, asset_template_t> data;

				Placeholder() = default;
				Placeholder(const asset_template_t & data)
					: data(data)
				{}
			};

			void post_load_level(std::string name, void (* callback)(std::string name, const Level & data));
			void post_load_placeholder(std::string name, void (* callback)(std::string name, const Placeholder & data));
		}
	}
}

#endif /* ENGINE_EXTERNAL_LOADER_HPP */
