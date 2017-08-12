
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/color.hpp>
#include <core/async/Thread.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/container/Stack.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>
#include <core/sync/Event.hpp>

#include <engine/Asset.hpp>
#include <engine/Command.hpp>
#include <engine/debug.hpp>
#include <engine/graphics/viewer.hpp>

#include <utility/any.hpp>
#include <utility/variant.hpp>

#include <atomic>
#include <fstream>
#include <utility>

using namespace engine::graphics::opengl;

namespace engine
{
namespace application
{
namespace window
{
	extern void make_current();
	extern void swap_buffers();
}
}
}

namespace gameplay
{
namespace gamestate
{
	extern void post_command(engine::Entity entity, engine::Command command, utility::any && data);
}
}

namespace
{
	class Stack
	{
	public:
		using this_type = Stack;
		using value_type = core::maths::Matrix4x4f;

		static constexpr std::size_t capacity = 10;
	private:
		using prime_type = value_type::value_type;

	private:
		core::container::Stack<value_type, capacity> stack;

	public:
		Stack()
		{
			this->stack.emplace();
		}

	public:
		void pop()
		{
			debug_assert(this->stack.size() > std::size_t{1});
			this->stack.pop();
		}
		void push()
		{
			debug_assert(this->stack.size() < capacity);
			this->stack.push(this->stack.top());
		}

		void load(const value_type matrix)
		{
			this->stack.top() = matrix;
		}
		void mult(const value_type matrix)
		{
			this->stack.top() *= matrix;
		}
		/**
		 * \note Use `mult` instead.
		 */
		void rotate(const core::maths::radian<prime_type> radian, const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::rotation(radian, x, y, z);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void scale(const prime_type xyz)
		{
			scale(xyz, xyz, xyz);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void scale(const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::scale(x, y, z);
		}
		/**
		 * \note Use `mult` instead.
		 */
		void translate(const prime_type x, const prime_type y, const prime_type z)
		{
			this->stack.top() *= value_type::translation(x, y, z);
		}

	public:
		friend void glLoadMatrix(const Stack & stack)
		{
			glLoadMatrix(stack.stack.top());
		}
	};
}

namespace
{
	struct color_t
	{
		engine::graphics::opengl::Color4ub color;

		color_t(unsigned int color)
			: color((color & 0x000000ff) >>  0,
			        (color & 0x0000ff00) >>  8,
			        (color & 0x00ff0000) >> 16,
			        (color & 0xff000000) >> 24)
		{
		}

		void enable() const
		{
			glColor(color);
		}
		void disable() const
		{
		}
	};

	engine::graphics::opengl::Color4ub color_from(const engine::Entity entity)
	{
		return engine::graphics::opengl::Color4ub{
			(entity & 0x000000ff) >> 0,
			(entity & 0x0000ff00) >> 8,
			(entity & 0x00ff0000) >> 16,
			(entity & 0xff000000) >> 24 };
	}

	engine::graphics::opengl::Color4ub color_from(const engine::graphics::data::Color color)
	{
		return engine::graphics::opengl::Color4ub{
			(color & 0x000000ff) >> 0,
			(color & 0x0000ff00) >> 8,
			(color & 0x00ff0000) >> 16,
			(color & 0xff000000) >> 24 };
	}

	struct texture_t
	{
		GLuint id;

		~texture_t()
		{
			glDeleteTextures(1, &id);
		}
		texture_t(core::graphics::Image && image)
		{
			glEnable(GL_TEXTURE_2D);

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT/*GL_CLAMP*/);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_LINEAR*/GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_NEAREST);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			switch (image.color())
			{
			case core::graphics::ImageColor::RGB:
				glTexImage2D(GL_TEXTURE_2D, 0, 3, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
				break;
			case core::graphics::ImageColor::RGBA:
				glTexImage2D(GL_TEXTURE_2D, 0, 4, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
				break;
			default:
				debug_unreachable();
			}

			glDisable(GL_TEXTURE_2D);
		}

		void enable() const
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, id);
		}
		void disable() const
		{
			glDisable(GL_TEXTURE_2D);
		}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		201,
		std::array<color_t, 100>,
		std::array<texture_t, 100>
	>
	materials;


	struct mesh_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		core::container::Buffer coords;
		std::vector<engine::model::weight_t> weights;

		mesh_t(engine::graphics::data::Mesh && data)
			: modelview(core::maths::Matrix4x4f::identity())
			, vertices(std::move(data.vertices))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.coords))
		{}

		mesh_t(engine::model::mesh_t && data)
			: modelview(data.matrix)
			, vertices(std::move(data.xyz))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.uv))
			, weights(std::move(data.weights))
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		401,
		std::array<mesh_t, 100>,
		std::array<mesh_t, 1>
	>
	resources;


	struct object_modelview
	{
		core::maths::Matrix4x4f modelview;

		object_modelview(
			core::maths::Matrix4x4f && modelview)
			: modelview(std::move(modelview))
		{}
	};
	struct object_modelview_vertices
	{
		core::maths::Matrix4x4f modelview;

		core::container::Buffer vertices;

		object_modelview_vertices(
			const mesh_t & mesh,
			core::maths::Matrix4x4f && modelview)
			: modelview(std::move(modelview))
			, vertices(mesh.vertices)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Entity,
		1601,
		std::array<object_modelview, 600>,
		std::array<object_modelview_vertices, 200>
	>
	objects;

	struct update_modelview
	{
		engine::graphics::data::ModelviewMatrix && data;

		void operator () ()
		{
		}
		template <typename T>
		void operator () (T & x)
		{
			x.modelview = std::move(data.matrix);
		}
	};


	struct comp_c
	{
		struct asset
		{
			const mesh_t * mesh;
			engine::graphics::opengl::Color4ub color;

			asset(engine::graphics::data::CompC::asset && data)
				: mesh(&resources.get<mesh_t>(data.mesh))
				, color((data.color & 0x000000ff) >> 0,
						(data.color & 0x0000ff00) >> 8,
						(data.color & 0x00ff0000) >> 16,
						(data.color & 0xff000000) >> 24)
			{}
		};

		object_modelview * object;
		std::vector<asset> assets;

		comp_c(
			object_modelview & object,
			std::vector<engine::graphics::data::CompC::asset> && assets)
			: object(&object)
		{
			this->assets.reserve(assets.size());
			for (auto & asset : assets)
			{
				this->assets.emplace_back(std::move(asset));
			}
		}
	};

	struct comp_t
	{
		const mesh_t * mesh;
		const texture_t * texture;
		object_modelview * object;

		comp_t(
			const mesh_t & mesh,
			const texture_t & texture,
			object_modelview & object)
			: mesh(&mesh)
			, texture(&texture)
			, object(&object)
		{}
	};

	struct Character
	{
		const mesh_t * mesh;
		const texture_t * texture;
		object_modelview_vertices * object;

		Character(
			const mesh_t & mesh,
			const texture_t & texture,
			object_modelview_vertices & object)
			: mesh(&mesh)
			, texture(&texture)
			, object(&object)
		{}

		void draw(const bool highlighted)
		{
			const mesh_t & mesh = *this->mesh;

			if (highlighted)
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(
					3, // TODO
				    GL_FLOAT, // TODO
				    0,
				    object->vertices.data());
				glNormalPointer(
					GL_FLOAT, // TODO
				    0,
				    mesh.normals.data());
				glDrawElements(
					GL_TRIANGLES,
					mesh.triangles.count(),
					GL_UNSIGNED_SHORT,
					mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				this->texture->enable();

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(
					3, // TODO
					GL_FLOAT, // TODO
					0,
					object->vertices.data());
				glNormalPointer(
					GL_FLOAT, // TODO
					0,
					mesh.normals.data());
				glTexCoordPointer(
					2, // TODO
					GL_FLOAT, // TODO
					0,
					mesh.coords.data());
				glDrawElements(
					GL_TRIANGLES,
					mesh.triangles.count(),
					GL_UNSIGNED_SHORT, // TODO
					mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);

				this->texture->disable();
			}
		}
	};

	struct Bar
	{
		Vector3f worldPosition;
		float progress;

		Bar(engine::graphics::data::Bar && bar)
			: worldPosition(bar.worldPosition)
			, progress(bar.progress)
		{}
	};

	struct linec_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer edges;
		engine::graphics::opengl::Color4ub color;

		engine::graphics::opengl::Color4ub selectable_color = {0, 0, 0, 0};

		linec_t(engine::graphics::data::LineC && data) :
			modelview(std::move(data.modelview)),
			vertices(std::move(data.vertices)),
			edges(std::move(data.edges)),
			color((data.color & 0x000000ff) >>  0,
			      (data.color & 0x0000ff00) >>  8,
			      (data.color & 0x00ff0000) >> 16,
			      (data.color & 0xff000000) >> 24)
		{}
		linec_t & operator = (engine::graphics::data::ModelviewMatrix && data)
		{
			modelview = std::move(data.matrix);
			return *this;
		}
	};

	namespace ui
	{
		struct PanelC
		{
			object_modelview * object;
			core::maths::Vector2f size;
			engine::graphics::opengl::Color4ub color;

			PanelC(
				object_modelview & object,
				core::maths::Vector2f && size,
				unsigned int && color)
				: object(&object)
				, size(std::move(size))
				, color(color_from(std::move(color)))
			{}
		};

		struct PanelT
		{
			const texture_t * texture;
			object_modelview * object;
			core::maths::Vector2f size;

			PanelT(
				const texture_t & texture,
				object_modelview & object,
				core::maths::Vector2f && size)
				: texture(&texture)
				, object(&object)
				, size(std::move(size))
			{}
		};

		struct Text
		{
			object_modelview * object;
			std::string display;
			engine::graphics::opengl::Color4ub color;

			Text(
				object_modelview & object,
				std::string && display,
				unsigned int && color)
				: object(&object)
				, display(std::move(display))
				, color(color_from(color))
			{}
		};
	}

	core::container::Collection
	<
		engine::Entity,
		1601,
		std::array<comp_c, 100>,
		std::array<comp_t, 100>,
		std::array<Character, 100>,
		std::array<Bar, 100>,
		std::array<linec_t, 100>,
		std::array<::ui::PanelC, 100>,
		std::array<::ui::PanelT, 100>,
		std::array<::ui::Text, 100>
	>
	components;


	struct selectable_character_t
	{
		const mesh_t * mesh;
		object_modelview_vertices * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_character_t(
			const mesh_t * mesh,
			object_modelview_vertices * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: mesh(mesh)
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_comp_c
	{
		std::vector<const mesh_t *> meshes;
		object_modelview * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_comp_c(
			std::vector<const mesh_t *> meshes,
			object_modelview * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: meshes(std::move(meshes))
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_comp_t
	{
		const mesh_t * mesh;
		object_modelview * object;

		engine::graphics::opengl::Color4ub selectable_color;

		selectable_comp_t(
			const mesh_t * mesh,
			object_modelview * object,
			engine::graphics::opengl::Color4ub selectable_color)
			: mesh(mesh)
			, object(object)
			, selectable_color(std::move(selectable_color))
		{}
	};

	struct selectable_panel
	{
		object_modelview * object;
		core::maths::Vector2f size;
		engine::graphics::opengl::Color4ub selectable_color;

		selectable_panel(
			object_modelview * object,
			core::maths::Vector2f size,
			engine::graphics::opengl::Color4ub selectable_color)
			: object(object)
			, size(std::move(size))
			, selectable_color(std::move(selectable_color))
		{}
	};

	core::container::Collection
	<
		engine::Entity,
		1601,
		std::array<selectable_character_t, 200>,
		std::array<selectable_comp_c, 250>,
		std::array<selectable_comp_t, 250>,
		std::array<selectable_panel, 100>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::Entity entity;
		engine::graphics::opengl::Color4ub color;

		void operator () (comp_c & x)
		{
			std::vector<const mesh_t *> meshes;
			meshes.reserve(x.assets.size());
			for (const auto & asset : x.assets)
			{
				meshes.push_back(asset.mesh);
			}
			selectable_components.emplace<selectable_comp_c>(entity, meshes, x.object, color);
		}
		void operator () (comp_t & x)
		{
			selectable_components.emplace<selectable_comp_t>(entity, x.mesh, x.object, color);
		}
		void operator () (Character & x)
		{
			selectable_components.emplace<selectable_character_t>(entity, x.mesh, x.object, color);
		}
		void operator () (ui::PanelC & x)
		{
			selectable_components.emplace<selectable_panel>(entity, x.object, x.size, color);
		}
		void operator () (ui::PanelT & x)
		{
			selectable_components.emplace<selectable_panel>(entity, x.object, x.size, color);
		}

		template <typename T>
		void operator () (T & x)
		{
			debug_unreachable();
		}
	};

	struct set_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		template <typename T>
		void operator () (T & x)
		{
			x.selectable_color = color;
		}
	};


	struct updateable_character_t
	{
		const mesh_t * mesh;
		object_modelview_vertices * object;

		std::vector<core::maths::Matrix4x4f> matrix_pallet;

		updateable_character_t(
			const mesh_t & mesh,
			object_modelview_vertices & object)
			: mesh(&mesh)
			, object(&object)
		{}

		void update()
		{
			const float * const untransformed_vertices = mesh->vertices.data_as<float>();
			float * const transformed_vertices = object->vertices.data_as<float>();
			const int nvertices = object->vertices.count() / 3; // assume xyz
			for (int i = 0; i < nvertices; i++)
			{
				const core::maths::Vector4f vertex = matrix_pallet[mesh->weights[i].index] * core::maths::Vector4f{untransformed_vertices[3 * i + 0], untransformed_vertices[3 * i + 1], untransformed_vertices[3 * i + 2], 1.f};
				core::maths::Vector4f::array_type buffer;
				vertex.get(buffer);
				transformed_vertices[3 * i + 0] = buffer[0];
				transformed_vertices[3 * i + 1] = buffer[1];
				transformed_vertices[3 * i + 2] = buffer[2];
				// assume buffer[3] == 1.f
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		1601,
		std::array<updateable_character_t, 200>,
		std::array<updateable_character_t, 1>
	>
	updateable_components;

	struct update_matrixpallet
	{
		engine::graphics::renderer::CharacterSkinning && data;

		void operator () ()
		{
		}
		void operator () (updateable_character_t & x)
		{
			x.matrix_pallet = std::move(data.matrix_pallet);
		}
	};
}

namespace
{
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
	using AssetMessage = utility::variant
	<
		MessageRegisterCharacter,
		MessageRegisterMesh,
		MessageRegisterTexture
	>;

	struct MessageAddBar
	{
		engine::Entity entity;
		engine::graphics::data::Bar bar;
	};
	struct MessageAddCharacterT
	{
		engine::Entity entity;
		engine::graphics::data::CompT component;
	};
	struct MessageAddComponentC
	{
		engine::Entity entity;
		engine::graphics::data::CompC component;
	};
	struct MessageAddComponentT
	{
		engine::Entity entity;
		engine::graphics::data::CompT component;
	};
	struct MessageAddLineC
	{
		engine::Entity entity;
		engine::graphics::data::LineC line;
	};
	struct MessageAddPanelC
	{
		engine::Entity entity;
		engine::graphics::data::ui::PanelC panel;
	};
	struct MessageAddPanelT
	{
		engine::Entity entity;
		engine::graphics::data::ui::PanelT panel;
	};
	struct MessageAddText
	{
		engine::Entity entity;
		engine::graphics::data::ui::Text text;
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
	struct MessageUpdatePanelC
	{
		engine::Entity entity;
		engine::graphics::data::ui::PanelC panel;
	};
	struct MessageUpdatePanelT
	{
		engine::Entity entity;
		engine::graphics::data::ui::PanelT panel;
	};
	struct MessageUpdateText
	{
		engine::Entity entity;
		engine::graphics::data::ui::Text text;
	};
	using EntityMessage = utility::variant
	<
		MessageAddBar,
		MessageAddCharacterT,
		MessageAddComponentC,
		MessageAddComponentT,
		MessageAddLineC,
		MessageAddPanelC,
		MessageAddPanelT,
		MessageAddText,
		MessageMakeObstruction,
		MessageMakeSelectable,
		MessageMakeTransparent,
		MessageRemove,
		MessageUpdateCharacterSkinning,
		MessageUpdateModelviewMatrix,
		MessageUpdatePanelC,
		MessageUpdatePanelT,
		MessageUpdateText
	>;

	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera2D> queue_notify_camera2d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Camera3D> queue_notify_camera3d;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Viewport> queue_notify_viewport;
	core::container::ExchangeQueueSRSW<engine::graphics::renderer::Cursor> queue_notify_cursor;

	core::container::CircleQueueSRMW<AssetMessage, 100> queue_assets;
	core::container::CircleQueueSRMW<EntityMessage, 1000> queue_entities;

	core::container::CircleQueueSRMW<std::tuple<int, int, engine::Entity>, 10> queue_select;

	void poll_queues()
	{
		AssetMessage asset_message;
		while (queue_assets.try_pop(asset_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageRegisterCharacter && x)
				{
					debug_assert(!resources.contains(x.asset));
					resources.emplace<mesh_t>(x.asset, std::move(x.mesh));
				}
				void operator () (MessageRegisterMesh && x)
				{
					debug_assert(!resources.contains(x.asset));
					resources.emplace<mesh_t>(x.asset, std::move(x.mesh));
				}
				void operator () (MessageRegisterTexture && x)
				{
					debug_assert(!materials.contains(x.asset));
					materials.emplace<texture_t>(x.asset, std::move(x.image));
				}
			};
			visit(ProcessMessage{}, std::move(asset_message));
		}

		EntityMessage entity_message;
		while (queue_entities.try_pop(entity_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageAddBar && x)
				{
					if (components.contains(x.entity))
					{
						// is this safe to do?
						auto & bar = components.get<Bar>(x.entity);
						bar.worldPosition = x.bar.worldPosition;
						bar.progress = x.bar.progress;
					}
					else
					{
						components.emplace<Bar>(x.entity, std::move(x.bar));
					}
				}
				void operator () (MessageAddCharacterT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & mesh = resources.get<mesh_t>(x.component.mesh);
					const auto & texture = materials.get<texture_t>(x.component.texture);
					auto & object = objects.emplace<object_modelview_vertices>(x.entity, mesh, std::move(x.component.modelview));
					components.emplace<Character>(x.entity, mesh, texture, object);
					updateable_components.emplace<updateable_character_t>(x.entity, mesh, object);
				}
				void operator () (MessageAddComponentC && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.component.modelview));
					components.emplace<comp_c>(x.entity, object, std::move(x.component.assets));
				}
				void operator () (MessageAddComponentT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & mesh = resources.get<mesh_t>(x.component.mesh);
					const auto & texture = materials.get<texture_t>(x.component.texture);
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.component.modelview));
					components.emplace<comp_t>(x.entity, mesh, texture, object);
				}
				void operator () (MessageAddLineC && x)
				{
					debug_assert(!components.contains(x.entity));
					components.emplace<linec_t>(x.entity, std::move(x.line));
				}
				void operator () (MessageAddPanelC && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.panel.matrix));
					components.emplace<::ui::PanelC>(x.entity, object, std::move(x.panel.size), std::move(x.panel.color));
				}
				void operator () (MessageAddPanelT && x)
				{
					debug_assert(!components.contains(x.entity));
					const auto & texture = materials.get<texture_t>(x.panel.texture);
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.panel.matrix));
					components.emplace<::ui::PanelT>(x.entity, texture, object, std::move(x.panel.size));
				}
				void operator () (MessageAddText && x)
				{
					debug_assert(!components.contains(x.entity));
					auto & object = objects.emplace<object_modelview>(x.entity, std::move(x.text.matrix));
					components.emplace<::ui::Text>(x.entity, object, std::move(x.text.display), std::move(x.text.color));
				}
				void operator () (MessageMakeObstruction && x)
				{
					const engine::graphics::opengl::Color4ub color = {0, 0, 0, 0};
					if (selectable_components.contains(x.entity))
					{
						selectable_components.call(x.entity, set_selectable_color{color});
					}
					else
					{
						components.call(x.entity, add_selectable_color{x.entity, color});
					}
				}
				void operator () (MessageMakeSelectable && x)
				{
					const engine::graphics::opengl::Color4ub color = {(x.entity & 0x000000ff) >> 0,
					                                                  (x.entity & 0x0000ff00) >> 8,
					                                                  (x.entity & 0x00ff0000) >> 16,
					                                                  (x.entity & 0xff000000) >> 24};
					if (selectable_components.contains(x.entity))
					{
						selectable_components.call(x.entity, set_selectable_color{color});
					}
					else
					{
						components.call(x.entity, add_selectable_color{x.entity, color});
					}
				}
				void operator () (MessageMakeTransparent && x)
				{
					if (selectable_components.contains(x.entity))
					{
						selectable_components.remove(x.entity);
					}
				}
				void operator () (MessageRemove && x)
				{
					debug_assert(components.contains(x.entity));
					if (selectable_components.contains(x.entity))
					{
						selectable_components.remove(x.entity);
					}
					if (updateable_components.contains(x.entity))
					{
						updateable_components.remove(x.entity);
					}
					components.remove(x.entity);
					if (objects.contains(x.entity))
					{
						objects.remove(x.entity);
					}
				}
				void operator () (MessageUpdateCharacterSkinning && x)
				{
					debug_assert(updateable_components.contains(x.entity));
					updateable_components.call(x.entity, update_matrixpallet{std::move(x.character_skinning)});
				}
				void operator () (MessageUpdateModelviewMatrix && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.call(x.entity, update_modelview{std::move(x.modelview_matrix)});
				}
				void operator () (MessageUpdatePanelC && x)
				{
					debug_assert(components.contains(x.entity));
					auto & view = components.get<ui::PanelC>(x.entity);
					view.object->modelview = x.panel.matrix;
					view.color = color_from(x.panel.color);
					view.size = x.panel.size;
				}
				void operator () (MessageUpdatePanelT && x)
				{
					debug_assert(components.contains(x.entity));
					const auto & texture = materials.get<texture_t>(x.panel.texture);
					auto & view = components.get<ui::PanelT>(x.entity);
					view.object->modelview = x.panel.matrix;
					view.size = x.panel.size;
					view.texture = &texture;
				}
				void operator () (MessageUpdateText && x)
				{
					debug_assert(components.contains(x.entity));
					auto & view = components.get<ui::Text>(x.entity);
					view.object->modelview = x.text.matrix;
					view.color = color_from(x.text.color);
					view.display = x.text.display;
				}
			};
			visit(ProcessMessage{}, std::move(entity_message));
		}
	}
}

namespace
{
	core::maths::Matrix4x4f projection2D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f projection3D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f view2D = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f view3D = core::maths::Matrix4x4f::identity();

	int viewport_x = 0;
	int viewport_y = 0;
	int viewport_width = 0;
	int viewport_height = 0;
	int cursor_x = -1;
	int cursor_y = -1;

	Stack modelview_matrix;

	engine::graphics::opengl::Font normal_font;

	GLuint framebuffer;
	GLuint entitybuffers[2]; // color, depth
	std::vector<uint32_t> entitypixels;
	std::atomic<int> entitytoggle;
	engine::Entity highlighted_entity = engine::Entity::null();
	engine::graphics::opengl::Color4ub highlighted_color{255, 191, 64, 255};

	engine::Entity get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = viewport_height - 1 - sy;

		if (x >= viewport_x &&
		    y >= viewport_y &&
		    x < viewport_x + viewport_width &&
		    y < viewport_y + viewport_height)
		{
			const unsigned int color = entitypixels[x + y * viewport_width];
			return engine::Entity{
				(color & 0xff000000) >> 24 |
				(color & 0x00ff0000) >> 8 |
				(color & 0x0000ff00) << 8 |
				(color & 0x000000ff) << 24};
		}
		return engine::Entity::null();
	}

	void initLights()
	{
		// set up light colors (ambient, diffuse, specular)
		GLfloat lightKa[] = { .5f, .5f, .5f, 1.f }; // ambient light
		GLfloat lightKd[] = { 0.f, 0.f, 0.f, 1.f }; // diffuse light
		GLfloat lightKs[] = { 0.f, 0.f, 0.f, 1.f }; // specular light
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

		// position the light
		float lightPos[4] = { 0, 0, 20, 1 }; // positional light
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glEnable(GL_LIGHT0);
	}

	template <typename T, std::size_t N>
	auto convert(std::array<T, N> && data)
	{
		core::container::Buffer buffer;
		buffer.resize<T>(N);

		std::copy(data.begin(), data.end(), buffer.data_as<T>());

		return buffer;
	}

	// TODO: move to loader/level
	auto createCuboid()
	{
		// the vertices of cuboid are numbered as followed:
		//              23--22
		//             /    /
		//            21--20
		//
		//        6  11--10   15
		//       /|  |    |   /|
		//      7 |  |    |  14|  18--19
		//      | 4  9----8  |13  |    |
		//      |/           |/   |    |
		//      5   0----1   12   16--17
		//   y     /    /
		//   |    2----3
		//   |
		//   +----x
		//  /
		// z
		const float xoffset = .5f;
		const float yoffset = .5f;
		const float zoffset = .5f;

		std::array<float, 3 * 24> va;
		std::size_t i = 0;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = -zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = -yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = +zoffset;
		va[i++] = +xoffset; va[i++] = +yoffset; va[i++] = -zoffset;
		va[i++] = -xoffset; va[i++] = +yoffset; va[i++] = -zoffset;

		core::container::Buffer vertices = convert(std::move(va));
		core::container::Buffer triangles = convert(std::array<uint16_t, 3 * 12> {{
				0, 1, 3,
				0, 3, 2,
				4, 5, 7,
				4, 7, 6,
				8, 9, 11,
				8, 11, 10,
				12, 13, 15,
				12, 15, 14,
				16, 17, 19,
				16, 19, 18,
				20, 21, 23,
				20, 23, 22}});
		core::container::Buffer normals = convert(std::array<float, 3 * 24> {{
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				0.f, -1.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				-1.f, 0.f, 0.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				0.f, 0.f, -1.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				+1.f, 0.f, 0.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, 0.f, +1.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f,
				0.f, +1.f, 0.f}});
		core::container::Buffer coords = convert(std::array<float, 2 * 24> {{
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f,
				0.f, 1.f,
				1.f, 1.f}});

		return std::move(engine::graphics::data::Mesh{
			vertices, triangles, normals, coords });
	}

	void render_setup()
	{
		graphics_debug_trace("render_callback starting");
		engine::application::window::make_current();

		graphics_debug_trace("glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		graphics_debug_trace("glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		graphics_debug_trace("glGetString GL_VERSION: ", glGetString(GL_VERSION));

		engine::graphics::opengl::init();

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		initLights();

		// entity buffers
		glGenFramebuffers(1, &framebuffer);
		glGenRenderbuffers(2, entitybuffers);

		// TODO: move to loader/level
		// vvvvvvvv tmp vvvvvvvv
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load("consolas", 12))
			{
				debug_fail();
			}
			normal_font.compile(data);

			data.free();
		}

		// add cuboid mesh as an asset
		engine::graphics::renderer::post_register_mesh(
			engine::Asset{ "cuboid" },
			createCuboid());

		core::graphics::Image image{"res/box.png"};
		engine::graphics::renderer::post_register_texture(engine::Asset{"my_png"}, std::move(image));

		core::graphics::Image image2{ "res/dude.png" };
		engine::graphics::renderer::post_register_texture(engine::Asset{ "dude" }, std::move(image2));

		core::graphics::Image image3{ "res/photo.png" };
		engine::graphics::renderer::post_register_texture(engine::Asset{ "photo" }, std::move(image3));

		engine::graphics::renderer::post_add_component(
			engine::Entity::create(),
			engine::graphics::data::CompT{
				core::maths::Matrix4x4f::translation(0.f, 5.f, 0.f),
				Vector3f{1.f, 1.f, 1.f},
				engine::Asset{ "cuboid" },
				engine::Asset{ "my_png" } });
		// ^^^^^^^^ tmp ^^^^^^^^
	}

	void render_update()
	{
		// poll events
		poll_queues();
		// update components
		for (auto & component : updateable_components.get<updateable_character_t>())
		{
			component.update();
		}
		//
		// read notifications
		//
		engine::graphics::renderer::Camera2D notification_camera2d;
		if (queue_notify_camera2d.try_pop(notification_camera2d))
		{
			projection2D = notification_camera2d.projection;
			view2D = notification_camera2d.view;
		}
		engine::graphics::renderer::Camera3D notification_camera3d;
		if (queue_notify_camera3d.try_pop(notification_camera3d))
		{
			projection3D = notification_camera3d.projection;
			view3D = notification_camera3d.view;
		}
		engine::graphics::renderer::Viewport notification_viewport;
		if (queue_notify_viewport.try_pop(notification_viewport))
		{
			viewport_x = notification_viewport.x;
			viewport_y = notification_viewport.y;

			glViewport(notification_viewport.x, notification_viewport.y, notification_viewport.width, notification_viewport.height);

			if (viewport_width != notification_viewport.width ||
			    viewport_height != notification_viewport.height)
			{
				viewport_width = notification_viewport.width;
				viewport_height = notification_viewport.height;

				// free old render buffers
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0); // color
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0); // depth
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				glDeleteRenderbuffers(2, entitybuffers);
				// allocate new render buffers
				glGenRenderbuffers(2, entitybuffers);
				glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[0]); // color
				glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, notification_viewport.width, notification_viewport.height);
				glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[1]); // depth
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, notification_viewport.width, notification_viewport.height);

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, entitybuffers[0]); // color
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, entitybuffers[1]); // depth
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

				entitypixels.resize(notification_viewport.width * notification_viewport.height);
			}
		}
		engine::graphics::renderer::Cursor notification_cursor;
		if (queue_notify_cursor.try_pop(notification_cursor))
		{
			cursor_x = notification_cursor.x;
			cursor_y = notification_cursor.y;
		}

		glStencilMask(0x000000ff);
		// setup frame
		glClearColor(0.f, 0.f, .1f, 0.f);
		glClearDepth(1.);
		//glClearStencil(0x00000000);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection3D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(view3D);

		////////////////////////////////////////////////////////
		//
		//  entity buffer
		//
		////////////////////////////////////////
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		// glViewport(...);
		glClearColor(0.f, 0.f, 0.f, 0.f); // null entity
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);

		for (const auto & component : selectable_components.get<selectable_character_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			glColor(component.selectable_color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                GL_FLOAT, // TODO
			                0,
			                component.object->vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.mesh->triangles.count(),
			               GL_UNSIGNED_SHORT,	// TODO
			               component.mesh->triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}
		for (const auto & component : selectable_components.get<selectable_comp_c>())
		{
			modelview_matrix.push();
			{
				modelview_matrix.mult(component.object->modelview);
				glLoadMatrix(modelview_matrix);

				glColor(component.selectable_color);
				glEnableClientState(GL_VERTEX_ARRAY);
				for (const auto & mesh : component.meshes)
				{
					glVertexPointer(3, // TODO
					                static_cast<GLenum>(mesh->vertices.format()), // TODO
					                0,
					                mesh->vertices.data());
					glDrawElements(GL_TRIANGLES,
					               mesh->triangles.count(),
					               static_cast<GLenum>(mesh->triangles.format()),
					               mesh->triangles.data());
				}
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			modelview_matrix.pop();
		}
		for (const auto & component : selectable_components.get<selectable_comp_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			glColor(component.selectable_color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.mesh->vertices.format()), // TODO
			                0,
			                component.mesh->vertices.data());
			glDrawElements(GL_TRIANGLES,
			               component.mesh->triangles.count(),
			               static_cast<GLenum>(component.mesh->triangles.format()),
			               component.mesh->triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection2D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(core::maths::Matrix4x4f::identity());

		for (const auto & component : selectable_components.get<selectable_panel>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			glColor(component.selectable_color);

			glBegin(GL_QUADS);
			{
				glVertex2f(0.f, size[1]);
				glVertex2f(size[0], size[1]);
				glVertex2f(size[0], 0.f);
				glVertex2f(0.f, 0.f);
			}
			glEnd();

			modelview_matrix.pop();
		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

		glReadPixels(viewport_x, viewport_y, viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, entitypixels.data());

		// hover
		{
			const engine::Entity entity = get_entity_at_screen(cursor_x, cursor_y);
			if (highlighted_entity != entity)
			{
				highlighted_entity = entity;
				gameplay::gamestate::post_command(engine::Entity::null(), engine::Command::RENDER_HIGHLIGHT, entity);
			}
		}
		// select
		{
			std::tuple<int, int, engine::Entity> select_args;
			while (queue_select.try_pop(select_args))
			{
				gameplay::gamestate::post_command(std::get<2>(select_args), engine::Command::RENDER_SELECT, get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)));
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		//////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection3D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(view3D);

		// wireframes
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_STENCIL_TEST);

		glDisable(GL_LIGHTING);

		glStencilMask(0x00000001);
		// glStencilFunc(GL_NEVER, 0x00000001, 0x00000001);
		// glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_ALWAYS, 0x00000001, 0x00000001);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		// TODO: make it possible to draw wireframes using meshes if needed.

		glEnable(GL_LIGHTING);

		//////////////////////////////////////////////////////////////
		// solids
		glStencilMask(0x00000000);
		glStencilFunc(GL_EQUAL, 0x00000000, 0x00000001);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		for (auto & component : components.get<Character>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			modelview_matrix.mult(component.mesh->modelview);
			glLoadMatrix(modelview_matrix);

			glColor(highlighted_color);

			// component.update();
			component.draw(components.get_key(component) == highlighted_entity);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<linec_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.modelview);
			glLoadMatrix(modelview_matrix);

			glLineWidth(2.f);
			glColor(components.get_key(component) == highlighted_entity ? highlighted_color : component.color);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, // TODO
			                static_cast<GLenum>(component.vertices.format()), // TODO
			                0,
			                component.vertices.data());
			glDrawElements(GL_LINES,
			               component.edges.count(),
			               static_cast<GLenum>(component.edges.format()),
			               component.edges.data());
			glDisableClientState(GL_VERTEX_ARRAY);
			glLineWidth(1.f);

			modelview_matrix.pop();
		}
		for (const auto & component : components.get<comp_t>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			const mesh_t & mesh = *component.mesh;

			if (components.get_key(component) == highlighted_entity)
			{
				glColor(highlighted_color);


				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(3, // TODO
				                static_cast<GLenum>(mesh.vertices.format()), // TODO
				                0,
				                mesh.vertices.data());
				glNormalPointer(static_cast<GLenum>(mesh.normals.format()), // TODO
				                0,
				                mesh.normals.data());
				glDrawElements(GL_TRIANGLES,
				               mesh.triangles.count(),
				               static_cast<GLenum>(mesh.triangles.format()),
				               mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				component.texture->enable();

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(3, // TODO
				                static_cast<GLenum>(mesh.vertices.format()), // TODO
				                0,
				                mesh.vertices.data());
				glNormalPointer(static_cast<GLenum>(mesh.normals.format()), // TODO
				                0,
				                mesh.normals.data());
				glTexCoordPointer(2, // TODO
				                  static_cast<GLenum>(mesh.vertices.format()), // TODO
				                  0,
				                  mesh.coords.data());
				glDrawElements(GL_TRIANGLES,
				               mesh.triangles.count(),
				               static_cast<GLenum>(mesh.triangles.format()),
				               mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);

				component.texture->disable();
			}

			modelview_matrix.pop();
		}
		for (const comp_c & component : components.get<comp_c>())
		{
			modelview_matrix.push();
			{
				modelview_matrix.mult(component.object->modelview);
				glLoadMatrix(modelview_matrix);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				for (const auto & a : component.assets)
				{
					const mesh_t & mesh = *a.mesh;

					glColor(components.get_key(component) == highlighted_entity ? highlighted_color : a.color);

					glVertexPointer(3, // TODO
					                static_cast<GLenum>(mesh.vertices.format()), // TODO
					                0,
					                mesh.vertices.data());
					glNormalPointer(static_cast<GLenum>(mesh.normals.format()), // TODO
					                0,
					                mesh.normals.data());
					glDrawElements(GL_TRIANGLES,
					               mesh.triangles.count(),
					               static_cast<GLenum>(mesh.triangles.format()),
					               mesh.triangles.data());
				}
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			modelview_matrix.pop();
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(projection2D);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(core::maths::Matrix4x4f::identity());

		glEnable(GL_DEPTH_TEST);

		// draw gui
		// ...
		glLoadMatrix(modelview_matrix);
		glColor3ub(255, 255, 255);
		glRasterPos2i(10, 10 + 12);
		normal_font.draw("herp derp herp derp herp derp herp derp herp derp etc.");

		for (const auto & component : ::components.get<::ui::PanelC>())
		{
			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			glColor(components.get_key(component) == highlighted_entity ? highlighted_color : component.color);

			glBegin(GL_QUADS);
			{
				glVertex2f(0.f, size[1]);
				glVertex2f(size[0], size[1]);
				glVertex2f(size[0], 0.f);
				glVertex2f(0.f, 0.f);
			}
			glEnd();

			modelview_matrix.pop();
		}

		for (const auto & component : ::components.get<::ui::PanelT>())
		{
			component.texture->enable();

			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			glLoadMatrix(modelview_matrix);

			core::maths::Vector2f::array_type size;
			component.size.get_aligned(size);

			glBegin(GL_QUADS);
			{
				glTexCoord2f(1.f, 0.f);
				glVertex2f(0.f, size[1]);
				glTexCoord2f(0.f, 0.f);
				glVertex2f(size[0], size[1]);
				glTexCoord2f(0.f, 1.f);
				glVertex2f(size[0], 0.f);
				glTexCoord2f(1.f, 1.f);
				glVertex2f(0.f, 0.f);
			}
			glEnd();

			modelview_matrix.pop();

			component.texture->disable();
		}

		for (const ::ui::Text & component : ::components.get<::ui::Text>())
		{
			glLoadMatrix(modelview_matrix);

			core::maths::Vector4f vec = component.object->modelview.get_column<3>();
			core::maths::Vector4f::array_type pos;
			vec.get_aligned(pos);

			glColor(component.color);
			glRasterPos3f(pos[0], pos[1], pos[2]);

			normal_font.draw(component.display);
		}

		glDisable(GL_DEPTH_TEST);

		// 2d
		// ...
		for (const Bar & component : components.get<Bar>())
		{
			modelview_matrix.push();

			// calculate modelview of bar position in world space
			core::maths::Vector2f screenCoord;
			engine::graphics::viewer::from_world_to_screen(component.worldPosition, screenCoord);
			core::maths::Vector2f::array_type b;
			screenCoord.get_aligned(b);

			Matrix4x4f modelview = make_translation_matrix(Vector3f{ b[0], b[1], 0.f });

			modelview_matrix.mult(modelview);
			glLoadMatrix(modelview_matrix);

			const float WF = 28.f;
			const float WI = WF - 2.f;
			const float HF = 5.f;
			const float HI = HF - 2.f;

			glBegin(GL_QUADS);
			{
				glColor3ub(20, 20, 40);

				glVertex2f(-WF, HF);
				glVertex2f(+WF, HF);
				glVertex2f(+WF,-HF);
				glVertex2f(-WF,-HF);

				glColor3ub(100, 255, 80);

				const float ws = -WI + component.progress * WI * 2;
				glVertex2f(-WI, HI);
				glVertex2f(ws, HI);
				glVertex2f(ws, -HI);
				glVertex2f(-WI, -HI);
			}
			glEnd();

			modelview_matrix.pop();
		}

		// entity buffers
		if (entitytoggle.load(std::memory_order_relaxed))
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

			glBlitFramebuffer(viewport_x, viewport_y, viewport_width, viewport_height, viewport_x, viewport_y, viewport_width, viewport_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		// swap buffers
		engine::application::window::swap_buffers();
	}

	void render_teardown()
	{
		graphics_debug_trace("render_callback stopping");
		// vvvvvvvv tmp vvvvvvvv
		{
			normal_font.decompile();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);
	}

	core::async::Thread renderThread;
	volatile bool active = true;
	core::sync::Event<true> event;

	void run()
	{
		render_setup();

		event.wait();
		event.reset();

		while (active)
		{
			render_update();

			event.wait();
			event.reset();
		}

		render_teardown();
	}
}

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void create()
			{
				renderThread = core::async::Thread{ run };
			}

			void update()
			{
				event.set();
			}

			void destroy()
			{
				active = false;
				event.set();

				renderThread.join();
			}

			void notify(Camera2D && data)
			{
				queue_notify_camera2d.try_push(std::move(data));
			}
			void notify(Camera3D && data)
			{
				queue_notify_camera3d.try_push(std::move(data));
			}
			void notify(Viewport && data)
			{
				queue_notify_viewport.try_push(std::move(data));
			}
			void notify(Cursor && data)
			{
				queue_notify_cursor.try_push(std::move(data));
			}

			void post_register_character(engine::Asset asset, engine::model::mesh_t && data)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterCharacter>, asset, std::move(data));
				debug_assert(res);
			}
			void post_register_mesh(engine::Asset asset, data::Mesh && data)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterMesh>, asset, std::move(data));
				debug_assert(res);
			}
			void post_register_texture(engine::Asset asset, core::graphics::Image && image)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterTexture>, asset, std::move(image));
				debug_assert(res);
			}

			void post_add_bar(engine::Entity entity, data::Bar && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddBar>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_character(engine::Entity entity, data::CompT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddCharacterT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_component(engine::Entity entity, data::CompC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddComponentC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_component(engine::Entity entity, data::CompT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddComponentT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_line(engine::Entity entity, data::LineC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddLineC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_panel(engine::Entity entity, data::ui::PanelC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddPanelC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_panel(engine::Entity entity, data::ui::PanelT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddPanelT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_text(engine::Entity entity, data::ui::Text && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddText>, entity, std::move(data));
				debug_assert(res);
			}

			void post_make_obstruction(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeObstruction>, entity);
				debug_assert(res);
			}
			void post_make_selectable(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeSelectable>, entity);
				debug_assert(res);
			}
			void post_make_transparent(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeTransparent>, entity);
				debug_assert(res);
			}

			void post_remove(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity);
				debug_assert(res);
			}

			void post_update_characterskinning(engine::Entity entity, CharacterSkinning && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateCharacterSkinning>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_modelviewmatrix(engine::Entity entity, data::ModelviewMatrix && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateModelviewMatrix>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_panel(engine::Entity entity, data::ui::PanelC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_panel(engine::Entity entity, data::ui::PanelT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_text(engine::Entity entity, data::ui::Text && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateText>, entity, std::move(data));
				debug_assert(res);
			}

			void post_select(int x, int y, engine::Entity entity)
			{
				const auto res = queue_select.try_emplace(x, y, entity);
				debug_assert(res);
			}

			void toggle_down()
			{
				entitytoggle.fetch_add(1, std::memory_order_relaxed);
			}
			void toggle_up()
			{
				entitytoggle.fetch_sub(1, std::memory_order_relaxed);
			}
		}
	}
}
