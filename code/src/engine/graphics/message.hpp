
#ifndef ENGINE_GRAPHICS_MESSAGE_HPP
#define ENGINE_GRAPHICS_MESSAGE_HPP

#include "core/graphics/Image.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/model/data.hpp"

#include "utility/variant.hpp"

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			struct MessageAddDisplay
			{
				engine::Asset asset;
				engine::graphics::renderer::display display;
			};

			struct MessageRemoveDisplay
			{
				engine::Asset asset;
			};

			struct MessageUpdateDisplayCamera2D
			{
				engine::Asset asset;
				engine::graphics::renderer::camera_2d camera_2d;
			};

			struct MessageUpdateDisplayCamera3D
			{
				engine::Asset asset;
				engine::graphics::renderer::camera_3d camera_3d;
			};

			struct MessageUpdateDisplayViewport
			{
				engine::Asset asset;
				engine::graphics::renderer::viewport viewport;
			};

			struct MessageRegisterCharacter
			{
				engine::Asset asset;
				engine::model::mesh_t mesh;
			};
			struct MessageRegisterMesh
			{
				engine::Asset asset;
				engine::graphics::data::Mesh mesh;
			};

			struct MessageRegisterTexture
			{
				engine::Asset asset;
				core::graphics::Image image;
			};

			struct MessageMakeObstruction
			{
				engine::Entity entity;
			};

			struct MessageMakeSelectable
			{
				engine::Entity entity;
			};

			struct MessageMakeTransparent
			{
				engine::Entity entity;
			};

			struct MessageMakeClearSelection
			{
			};

			struct MessageMakeDehighlighted
			{
				engine::Entity entity;
			};

			struct MessageMakeDeselect
			{
				engine::Entity entity;
			};

			struct MessageMakeHighlighted
			{
				engine::Entity entity;
			};

			struct MessageMakeSelect
			{
				engine::Entity entity;
			};

			struct MessageRemove
			{
				engine::Entity entity;
			};

			struct MessageUpdateCharacterSkinning
			{
				engine::Entity entity;
				engine::graphics::renderer::CharacterSkinning character_skinning;
			};

			struct MessageUpdateModelviewMatrix
			{
				engine::Entity entity;
				engine::graphics::data::ModelviewMatrix modelview_matrix;
			};

			using Message = utility::variant
			<
				MessageAddDisplay,
				MessageRemoveDisplay,
				MessageUpdateDisplayCamera2D,
				MessageUpdateDisplayCamera3D,
				MessageUpdateDisplayViewport,

				MessageRegisterCharacter,
				MessageRegisterMesh,
				MessageRegisterTexture,

				MessageMakeObstruction,
				MessageMakeSelectable,
				MessageMakeTransparent,
				MessageMakeClearSelection,
				MessageMakeDehighlighted,
				MessageMakeDeselect,
				MessageMakeHighlighted,
				MessageMakeSelect,
				MessageRemove,
				MessageUpdateCharacterSkinning,
				MessageUpdateModelviewMatrix
			>;
		}
	}
}

#endif /* ENGINE_GRAPHICS_MESSAGE_HPP */
