
#ifndef ENGINE_RESOURCE_READER_HPP
#define ENGINE_RESOURCE_READER_HPP

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
		namespace data
		{
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
			// using Placeholder = utility::variant<engine::model::mesh_t, json>;
			struct Placeholder
			{
				struct Header
				{
					std::string name;
					engine::Asset asset;

					Header() = default;
					Header(const std::string & name)
						: name(name)
						, asset(name)
					{}
				} header;
				utility::variant<engine::model::mesh_t, json> data;

				Placeholder() = default;
				Placeholder(const std::string & name, engine::model::mesh_t && data)
					: header(name)
					, data(std::move(data))
				{}
				Placeholder(const std::string & name, json && data)
					: header(name)
					, data(std::move(data))
				{}
			};
		}

		void post_read_level(const std::string & filename, void (* callback)(const std::string & filename, data::Level && data));
		void post_read_placeholder(const std::string & filename, void (* callback)(const std::string & filename, data::Placeholder && data));
	}
}

#endif /* ENGINE_RESOURCE_READER_HPP */
