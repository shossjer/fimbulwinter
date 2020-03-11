
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include "core/container/Buffer.hpp"
#include "core/graphics/Image.hpp"
#include "core/maths/Matrix.hpp"
#include "core/serialization.hpp"

#include "engine/Command.hpp"
#include "engine/Entity.hpp"
#include "engine/common.hpp"
#include "engine/model/data.hpp"

#include "utility/any.hpp"
// todo forward declare
#include "utility/unicode.hpp"

#include <vector>

namespace engine
{
	namespace application
	{
		class window;
	}

	namespace file
	{
		class system;
	}
}

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
				core::maths::Vector3f scale;
				engine::Asset mesh;
				engine::Asset texture;
			};

			struct CompC
			{
				core::maths::Matrix4x4f modelview;
				core::maths::Vector3f scale;

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

				core::maths::Vector3f worldPosition;

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

		class renderer
		{
		public:
			struct camera_2d
			{
				core::maths::Matrix4x4f projection;
				core::maths::Matrix4x4f view;
			};
			struct camera_3d
			{
				core::maths::Matrix4x4f projection;
				core::maths::Matrix4x4f frame;
				core::maths::Matrix4x4f view;
				core::maths::Matrix4x4f inv_projection;
				core::maths::Matrix4x4f inv_frame;
				core::maths::Matrix4x4f inv_view;
			};
			struct viewport
			{
				int32_t x;
				int32_t y;
				int32_t width;
				int32_t height;
			};
			struct display
			{
				viewport viewport;
				camera_3d camera_3d;
				camera_2d camera_2d;
			};

			struct Cursor
			{
				int32_t x;
				int32_t y;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_view("x"), &Cursor::x),
						std::make_pair(utility::string_view("y"), &Cursor::y)
						);
				}
			};

			struct SelectData
			{
				engine::Entity entity;
				Cursor cursor;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_view("entity"), &SelectData::entity),
						std::make_pair(utility::string_view("cursor"), &SelectData::cursor)
						);
				}
			};

			struct CharacterSkinning
			{
				std::vector<core::maths::Matrix4x4f> matrix_pallet;
			};

			enum struct Type
			{
				OPENGL_1_2,
				OPENGL_3_0
			};

		public:
			~renderer();
			renderer(engine::application::window & window, engine::file::system & filesystem, void (* callback_select)(engine::Entity entity, engine::Command command, utility::any && data), Type type);
		};

		constexpr auto serialization(utility::in_place_type_t<renderer::Type>)
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("opengl1.2"), renderer::Type::OPENGL_1_2),
				std::make_pair(utility::string_view("opengl3.0"), renderer::Type::OPENGL_3_0)
				);
		}

		// todo this feels hacky
		void set_shader_directory(renderer & renderer, utility::heap_string_utf8 && directory);

		// void notify(renderer & renderer, renderer::Camera2D && data);
		// void notify(renderer & renderer, renderer::Camera3D && data);
		// void notify(renderer & renderer, renderer::Viewport && data);
		void post_add_display(renderer & renderer, engine::Asset asset, renderer::display && data);
		void post_remove_display(renderer & renderer, engine::Asset asset);
		void post_update_display(renderer & renderer, engine::Asset asset, renderer::camera_2d && data);
		void post_update_display(renderer & renderer, engine::Asset asset, renderer::camera_3d && data);
		void post_update_display(renderer & renderer, engine::Asset asset, renderer::viewport && data);

		void post_register_character(renderer & renderer, engine::Asset asset, engine::model::mesh_t && data);
		void post_register_mesh(renderer & renderer, engine::Asset asset, data::Mesh && data);
		void post_register_texture(renderer & renderer, engine::Asset asset, core::graphics::Image && image);

		void post_add_bar(renderer & renderer, engine::Entity entity, data::Bar && bar);
		void post_add_character(renderer & renderer, engine::Entity entity, data::CompT && data);
		void post_add_component(renderer & renderer, engine::Entity entity, data::CompC && data);
		void post_add_component(renderer & renderer, engine::Entity entity, data::CompT && data);
		void post_add_line(renderer & renderer, engine::Entity entity, data::LineC && data);
		void post_add_panel(renderer & renderer, engine::Entity entity, data::ui::PanelC && data);
		void post_add_panel(renderer & renderer, engine::Entity entity, data::ui::PanelT && data);
		void post_add_text(renderer & renderer, engine::Entity entity, data::ui::Text && data);

		void post_make_selectable(renderer & renderer, engine::Entity entity);
		void post_make_obstruction(renderer & renderer, engine::Entity entity);
		void post_make_transparent(renderer & renderer, engine::Entity entity);

		void post_make_clear_selection(renderer & renderer);
		void post_make_dehighlight(renderer & renderer, engine::Entity entity);
		void post_make_deselect(renderer & renderer, engine::Entity entity);
		void post_make_highlight(renderer & renderer, engine::Entity entity);
		void post_make_select(renderer & renderer, engine::Entity entity);

		void post_remove(renderer & renderer, engine::Entity entity);

		void post_update_characterskinning(renderer & renderer, engine::Entity entity, renderer::CharacterSkinning && data);
		void post_update_modelviewmatrix(renderer & renderer, engine::Entity entity, data::ModelviewMatrix && data);
		void post_update_panel(renderer & renderer, engine::Entity entity, data::ui::PanelC && data);
		void post_update_panel(renderer & renderer, engine::Entity entity, data::ui::PanelT && data);
		void post_update_text(renderer & renderer, engine::Entity entity, data::ui::Text && data);

		void post_select(renderer & renderer, int x, int y, engine::Entity entity, engine::Command command);

		void toggle_down(renderer & renderer);
		void toggle_up(renderer & renderer);
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
