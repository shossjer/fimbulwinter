#pragma once

#include "core/graphics/Image.hpp"

#include "engine/graphics/renderer.hpp"
#include "engine/model/data.hpp"
#include "engine/Token.hpp"

#include "utility/variant.hpp"

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			struct MessageAddDisplay
			{
				engine::Token asset;
				engine::graphics::data::display display;
			};

			struct MessageRemoveDisplay
			{
				engine::Token asset;
			};

			struct MessageUpdateDisplayCamera2D
			{
				engine::Token asset;
				engine::graphics::data::camera_2d camera_2d;
			};

			struct MessageUpdateDisplayCamera3D
			{
				engine::Token asset;
				engine::graphics::data::camera_3d camera_3d;
			};

			struct MessageUpdateDisplayViewport
			{
				engine::Token asset;
				engine::graphics::data::viewport viewport;
			};

			struct MessageRegisterCharacter
			{
				engine::Token asset;
				engine::model::mesh_t mesh;
			};

			struct MessageRegisterMaterial
			{
				engine::Token asset;
				engine::graphics::data::MaterialAsset material;
			};

			struct MessageRegisterMesh
			{
				engine::Token asset;
				engine::graphics::data::MeshAsset mesh;
			};

			struct MessageRegisterTexture
			{
				engine::Token asset;
				core::graphics::Image image;
			};

			struct MessageCreateMaterialInstance
			{
				engine::Token entity;
				engine::graphics::data::MaterialInstance data;
			};

			struct MessageDestroy
			{
				engine::Token entity;
			};

			struct MessageAddMeshObject
			{
				engine::Token entity;
				engine::graphics::data::MeshObject object;
			};

			struct MessageMakeObstruction
			{
				engine::Token entity;
			};

			struct MessageMakeSelectable
			{
				engine::Token entity;
			};

			struct MessageMakeTransparent
			{
				engine::Token entity;
			};

			struct MessageMakeClearSelection
			{
			};

			struct MessageMakeDehighlighted
			{
				engine::Token entity;
			};

			struct MessageMakeDeselect
			{
				engine::Token entity;
			};

			struct MessageMakeHighlighted
			{
				engine::Token entity;
			};

			struct MessageMakeSelect
			{
				engine::Token entity;
			};

			struct MessageRemove
			{
				engine::Token entity;
			};

			struct MessageUpdateCharacterSkinning
			{
				engine::Token entity;
				engine::graphics::data::CharacterSkinning character_skinning;
			};

			struct MessageUpdateModelviewMatrix
			{
				engine::Token entity;
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
				MessageRegisterMaterial,
				MessageRegisterMesh,
				MessageRegisterTexture,

				MessageCreateMaterialInstance,
				MessageDestroy,

				MessageAddMeshObject,
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
