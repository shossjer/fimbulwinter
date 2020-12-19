#pragma once

#include "core/container/Buffer.hpp"
#include "core/graphics/Image.hpp"
#include "core/maths/Matrix.hpp"
#include "core/serialization.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/common.hpp"
#include "engine/model/data.hpp"
#include "engine/Token.hpp"

#include "utility/optional.hpp"
// todo forward declare
#include "utility/unicode/string.hpp"

#include <vector>

namespace engine
{
	namespace application
	{
		class window;
	}
}

namespace utility
{
	class any;
}

namespace engine
{
	namespace graphics
	{
		class renderer
		{
		public:
			enum struct Type
			{
				OPENGL_1_2,
				OPENGL_3_0
			};

		public:
			~renderer();
			explicit renderer(engine::application::window & window, void (*callback_select)(engine::Token entity, engine::Command command, utility::any && data), Type type);
		};

		constexpr auto serialization(utility::in_place_type_t<renderer::Type>)
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("opengl1.2"), renderer::Type::OPENGL_1_2),
				std::make_pair(utility::string_units_utf8("opengl3.0"), renderer::Type::OPENGL_3_0)
				);
		}

		namespace data
		{
			struct Cursor
			{
				int32_t x;
				int32_t y;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("x"), &Cursor::x),
						std::make_pair(utility::string_units_utf8("y"), &Cursor::y)
					);
				}
			};

			struct SelectData
			{
				engine::Token entity;
				Cursor cursor;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("entity"), &SelectData::entity),
						std::make_pair(utility::string_units_utf8("cursor"), &SelectData::cursor)
					);
				}
			};

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

			struct MaterialAsset
			{
				utility::optional<uint32_t> diffuse;
				utility::optional<engine::Asset> shader;

				static constexpr auto serialization()
				{
					return utility::make_lookup_table(
						std::make_pair(utility::string_units_utf8("diffuse"), &MaterialAsset::diffuse),
						std::make_pair(utility::string_units_utf8("shader"), &MaterialAsset::shader)
					);
				}
			};

			struct ShaderData
			{
				struct FragmentOutput
				{
					utility::heap_string_utf8 name;
					int value;

					static constexpr auto serialization()
					{
						return make_lookup_table(
							std::make_pair(utility::string_units_utf8("name"), &FragmentOutput::name),
							std::make_pair(utility::string_units_utf8("value"), &FragmentOutput::value)
						);
					}
				};

				struct VertexInput
				{
					utility::heap_string_utf8 name;
					int value;

					static constexpr auto serialization()
					{
						return make_lookup_table(
							std::make_pair(utility::string_units_utf8("name"), &VertexInput::name),
							std::make_pair(utility::string_units_utf8("value"), &VertexInput::value)
						);
					}
				};

				std::vector<VertexInput> inputs;
				std::vector<FragmentOutput> outputs;
				utility::heap_string_utf8 vertex_source;
				utility::heap_string_utf8 fragment_source;

				static constexpr auto serialization()
				{
					return make_lookup_table(
						std::make_pair(utility::string_units_utf8("inputs"), &ShaderData::inputs),
						std::make_pair(utility::string_units_utf8("outputs"), &ShaderData::outputs),
						std::make_pair(utility::string_units_utf8("vertex_source"), &ShaderData::vertex_source),
						std::make_pair(utility::string_units_utf8("fragment_source"), &ShaderData::fragment_source)
					);
				}
			};

			struct MeshAsset
			{
				core::container::Buffer vertices;
				core::container::Buffer triangles;
				core::container::Buffer normals;
				core::container::Buffer coords;
			};

			struct MaterialInstance
			{
				struct Texture
				{
					engine::Token texture;
				};

				engine::Token materialclass;
				uint32_t diffuse;
				std::vector<Texture> textures;
			};

			struct MeshObject
			{
				core::maths::Matrix4x4f matrix;

				engine::Token mesh;
				engine::Token material;
			};

			struct CharacterSkinning
			{
				std::vector<core::maths::Matrix4x4f> matrix_pallet;
			};

			struct ModelviewMatrix
			{
				core::maths::Matrix4x4f matrix;
			};
		}

		// void notify(renderer & renderer, renderer::Camera2D && data);
		// void notify(renderer & renderer, renderer::Camera3D && data);
		// void notify(renderer & renderer, renderer::Viewport && data);
		void post_add_display(renderer & renderer, engine::Token asset, data::display && data);
		void post_remove_display(renderer & renderer, engine::Token asset);
		void post_update_display(renderer & renderer, engine::Token asset, data::camera_2d && data);
		void post_update_display(renderer & renderer, engine::Token asset, data::camera_3d && data);
		void post_update_display(renderer & renderer, engine::Token asset, data::viewport && data);

		void post_register_character(renderer & renderer, engine::Token asset, engine::model::mesh_t && data);
		void post_register_material(renderer & renderer, engine::Token asset, data::MaterialAsset && data);
		void post_register_mesh(renderer & renderer, engine::Token asset, data::MeshAsset && data);
		void post_register_shader(renderer & renderer, engine::Token asset, data::ShaderData && data);
		void post_register_texture(renderer & renderer, engine::Token asset, core::graphics::Image && image);
		void post_unregister(renderer & renderer, engine::Token asset);

		void post_create_material(renderer & renderer, engine::Token entity, data::MaterialInstance && data);
		void post_destroy(renderer & renderer, engine::Token entity);

		void post_add_object(renderer & renderer, engine::Token entity, data::MeshObject && data);
		void post_remove_object(renderer & renderer, engine::Token entity);

		void post_make_selectable(renderer & renderer, engine::Token entity);
		void post_make_obstruction(renderer & renderer, engine::Token entity);
		void post_make_transparent(renderer & renderer, engine::Token entity);

		void post_make_clear_selection(renderer & renderer);
		void post_make_dehighlight(renderer & renderer, engine::Token entity);
		void post_make_deselect(renderer & renderer, engine::Token entity);
		void post_make_highlight(renderer & renderer, engine::Token entity);
		void post_make_select(renderer & renderer, engine::Token entity);

		void post_remove(renderer & renderer, engine::Token entity);

		void post_update_characterskinning(renderer & renderer, engine::Token entity, data::CharacterSkinning && data);
		void post_update_modelviewmatrix(renderer & renderer, engine::Token entity, data::ModelviewMatrix && data);

		void post_select(renderer & renderer, int x, int y, engine::Token entity, engine::Command command);

		void toggle_down(renderer & renderer);
		void toggle_up(renderer & renderer);
	}
}
