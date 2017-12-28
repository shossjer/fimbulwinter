
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include "engine/Command.hpp"
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
			};

			struct Bar
			{
				// TODO: enum for type of Bar (Progress, Hitpoints, Level etc)

				Vector3f worldPosition;

				// value between 0.f to 1.f
				float progress;
			};

			namespace ui
			{
				struct PanelC
				{
					core::maths::Matrix4x4f matrix;
					core::maths::Vector2f size;
					Color color;
				};

				struct PanelT
				{
					core::maths::Matrix4x4f matrix;
					core::maths::Vector2f size;
					engine::Asset texture;
				};

				struct Text
				{
					core::maths::Matrix4x4f matrix;
					Color color;
					std::string display;
					// TODO: asset font resource
					// TODO: font size
				};
			}

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
			};
			struct Camera3D
			{
				core::maths::Matrix4x4f projection;
				core::maths::Matrix4x4f view;
				core::maths::Matrix4x4f inv_projection;
				core::maths::Matrix4x4f inv_view;
			};
			struct Viewport
			{
				int32_t x;
				int32_t y;
				int32_t width;
				int32_t height;
			};
			struct Cursor
			{
				int32_t x;
				int32_t y;
			};

			struct CharacterSkinning
			{
				std::vector<core::maths::Matrix4x4f> matrix_pallet;
			};

			void create();
			void destroy();

			// void notify(Camera2D && data);
			// void notify(Camera3D && data);
			// void notify(Viewport && data);

			void post_register_character(engine::Asset asset, engine::model::mesh_t && data);
			void post_register_mesh(engine::Asset asset, data::Mesh && data);
			void post_register_texture(engine::Asset asset, core::graphics::Image && image);

			void post_add_bar(engine::Entity entity, data::Bar && bar);
			void post_add_character(engine::Entity entity, data::CompT && data);
			void post_add_component(engine::Entity entity, data::CompC && data);
			void post_add_component(engine::Entity entity, data::CompT && data);
			void post_add_line(engine::Entity entity, data::LineC && data);
			void post_add_panel(engine::Entity entity, data::ui::PanelC && data);
			void post_add_panel(engine::Entity entity, data::ui::PanelT && data);
			void post_add_text(engine::Entity entity, data::ui::Text && data);

			void post_make_selectable(engine::Entity entity);
			void post_make_obstruction(engine::Entity entity);
			void post_make_transparent(engine::Entity entity);

			void post_make_clear_selection();
			void post_make_deselect(engine::Entity entity);
			void post_make_select(engine::Entity entity);

			void post_remove(engine::Entity entity);

			// void post_update_characterskinning(engine::Entity entity, CharacterSkinning && data);
			void post_update_modelviewmatrix(engine::Entity entity, data::ModelviewMatrix && data);
			void post_update_panel(engine::Entity entity, data::ui::PanelC && data);
			void post_update_panel(engine::Entity entity, data::ui::PanelT && data);
			void post_update_text(engine::Entity entity, data::ui::Text && data);

			void post_select(int x, int y, engine::Entity entity, engine::Command command);

			void toggle_down();
			void toggle_up();
		}
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
