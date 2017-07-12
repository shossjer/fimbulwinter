
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include <engine/common.hpp>
#include <engine/model/data.hpp>

#include <core/container/Buffer.hpp>
#include <core/graphics/Image.hpp>
#include <core/maths/Matrix.hpp>

#include <vector>

namespace engine
{
	namespace graphics
	{
		namespace data
		{
			// Color
			using Color = uint32_t;

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

			// mesh with optional uv-coords
			// uv-coords must exist if any component instance has texture
			struct Mesh
			{
				core::container::Buffer vertices;
				core::container::Buffer triangles;
				core::container::Buffer normals;
				core::container::Buffer coords;
			};

			// used when adding components
			struct CompT
			{
				core::maths::Matrix4x4f modelview;
				Vector3f scale;
				engine::Asset mesh;
				engine::Asset texture;
			};

			struct CompC
			{
				core::maths::Matrix4x4f modelview;
				Vector3f scale;

				struct asset
				{
					engine::Asset mesh;
					Color color;
				};
				std::vector<asset> assets;

				CompC(core::maths::Matrix4x4f modelview,
					Vector3f scale,
					std::vector<asset> assets)
					: modelview(modelview)
					, scale(scale)
					, assets(assets)
				{}

				CompC()
					: modelview()
				{}
			};

			struct Bar
			{
				// TODO: enum for type of Bar (Progress, Hitpoints, Level etc)

				Vector3f worldPosition;

				// value between 0.f to 1.f
				float progress;
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
			struct Cursor
			{
				int32_t x;
				int32_t y;

				Cursor() {}
				Cursor(int32_t x, int32_t y) : x(x), y(y) {}
			};

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

			// add Assets (Materials and Resources)
			void post_register_texture(engine::Asset asset, const core::graphics::Image & image);

			void add(engine::Asset asset, data::Mesh && data);
			void add(engine::Asset asset, engine::model::mesh_t && data);

			// add and remove Entities
			void add(engine::Entity entity, data::LineC data);

			void add(engine::Entity entity, data::CompT && data);
			void add(engine::Entity entity, data::CompC && data);
			void add_character_instance(
				     engine::Entity entity, data::CompT && data);
			void add(engine::Entity entity, data::Bar && bar);

			void remove(engine::Entity entity);

			// update Entities
			void update(engine::Entity entity, data::ModelviewMatrix data);
			// void update(engine::Entity entity, CharacterSkinning data);

			void post_select(int x, int y, engine::Entity entity);

			void toggle_down();
			void toggle_up();
		}
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
