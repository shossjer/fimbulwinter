
#ifndef ENGINE_RESOURCE_READER_HPP
#define ENGINE_RESOURCE_READER_HPP

#include "core/JsonStructurer.hpp"
#include <core/container/Buffer.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/model/data.hpp>

#include <utility/json.hpp>
#include <utility/variant.hpp>

#include <string>
#include <vector>

namespace engine
{
	namespace resource
	{
		namespace reader
		{
			struct Data
			{
				core::JsonStructurer structurer;
			};

			struct Level
			{
				struct Mesh
				{
					std::string name;
					core::maths::Matrix4x4f matrix;
					float color[3];
					core::container::Buffer vertices;
					core::container::Buffer triangles;
					core::container::Buffer normals;
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
				utility::variant<engine::model::mesh_t, json> data;

				Placeholder() = default;
				Placeholder(engine::model::mesh_t && data)
					: data(std::move(data))
				{}
				Placeholder(json && data)
					: data(std::move(data))
				{}
			};

			void post_read_data(std::string name, void (* callback)(std::string name, Data && data));
			void post_read_level(std::string name, void (* callback)(std::string name, Level && data));
			void post_read_placeholder(std::string name, void (* callback)(std::string name, Placeholder && data));
		}
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
