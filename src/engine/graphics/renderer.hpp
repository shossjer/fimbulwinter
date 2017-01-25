
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
				bool wireframe;
			};
			// line segments with color
			struct LineC
			{
				core::maths::Matrix4x4f modelview;
				core::container::Buffer vertices;
				core::container::Buffer edges;
				Color color;

				LineC(const core::maths::Matrix4x4f & modelview,
					const core::container::Buffer & vertices,
					const core::container::Buffer & edges,
					const Color & color)
					:
					modelview(modelview),
					vertices(vertices),
					edges(edges),
					color(color)
				{}

				LineC()
				{}
			};
			// mesh with color
			struct MeshC
			{
				core::container::Buffer vertices;
				core::container::Buffer triangles;
				core::container::Buffer normals;
				Color color;
			};

			// modelview matrix
			struct ModelviewMatrix
			{
				core::maths::Matrix4x4f matrix;
			};
		}

		namespace renderer
		{
			struct Camera2D
			{
				core::maths::Matrix4x4f projection;
				core::maths::Matrix4x4f view;

				Camera2D() {}
				Camera2D(core::maths::Matrix4x4f projection,
				         core::maths::Matrix4x4f view) :
					projection(projection),
					view(view)
				{}
			};
			struct Camera3D
			{
				core::maths::Matrix4x4f projection;
				core::maths::Matrix4x4f view;
				core::maths::Matrix4x4f inv_projection;
				core::maths::Matrix4x4f inv_view;

				Camera3D() {}
				Camera3D(core::maths::Matrix4x4f projection,
				         core::maths::Matrix4x4f view,
				         core::maths::Matrix4x4f inv_projection,
				         core::maths::Matrix4x4f inv_view) :
					projection(projection),
					view(view),
					inv_projection(inv_projection),
					inv_view(inv_view)
				{}
			};
			struct Viewport
			{
				int32_t x;
				int32_t y;
				int32_t width;
				int32_t height;

				Viewport() {}
				Viewport(int32_t x, int32_t y, int32_t width, int32_t height) : x(x), y(y), width(width), height(height) {}
			};

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

			// void notify(Camera2D && data);
			// void notify(Camera3D && data);
			// void notify(Viewport && data);

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
