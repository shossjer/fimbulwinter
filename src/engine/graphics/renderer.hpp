
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include <core/container/Buffer.hpp>
#include <core/maths/Matrix.hpp>

#include <engine/Entity.hpp>

#include <vector>

namespace engine
{
	namespace graphics
	{
		namespace data
		{
			// Color
			using Color = uint32_t;

			// cuboid with color
			struct CuboidC
			{
				core::maths::Matrix4x4f modelview;
				float width, height, depth;
				Color color;
			};
			// line segments with color
			struct LineC
			{
				core::container::Buffer vertices;
				core::container::Buffer edges;
				Color color;
			};
			// mesh with color
			struct MeshC
			{
				core::container::Buffer vertices;
				core::container::Buffer triangles;
				core::container::Buffer normals;
				Color color;
			};


			// Data
			struct Data {};

			// modelview matrix
			struct ModelviewMatrix : Data
			{
				core::maths::Matrix4x4f matrix;
			};
		}

		namespace renderer
		{
			namespace asset
			{
				struct CharacterMesh
				{
					std::string mshfile;

					CharacterMesh() = default;
					CharacterMesh(std::string mshfile) :
						mshfile(mshfile)
					{}
				};
			}
			struct CharacterSkinning
			{
				std::vector<core::maths::Matrix4x4f> matrix_pallet;

				CharacterSkinning() = default;
				CharacterSkinning(std::vector<core::maths::Matrix4x4f> matrix_pallet) :
					matrix_pallet(std::move(matrix_pallet))
				{}
			};

			void create();
			void destroy();

			void add(engine::Entity entity, data::CuboidC data);
			void add(engine::Entity entity, data::LineC data);
			void add(engine::Entity entity, data::MeshC data);
			void add(engine::Entity entity, asset::CharacterMesh data);
			void remove(engine::Entity entity);
			void update(engine::Entity entity, data::ModelviewMatrix data);
			// void update(engine::Entity entity, CharacterSkinning data);
		}
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
