#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include "config.h"

#include "core/color.hpp"
#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"
#include "core/container/Stack.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"
#include "core/PngStructurer.hpp"
#include "core/sync/Event.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"

#include "utility/any.hpp"
#include "utility/profiling.hpp"
#include "utility/ranges.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>
#include <utility>

using namespace engine::graphics::opengl;

namespace engine
{
	namespace application
	{
		extern void make_current(window & window);
		extern void swap_buffers(window & window);
	}

	namespace graphics
	{
		namespace detail
		{
			extern core::async::Thread renderThread;
			extern std::atomic_int active;
			extern core::sync::Event<true> event;

			extern core::container::PageQueue<utility::heap_storage<Message>> message_queue;

			extern core::container::PageQueue<utility::heap_storage<int, int, engine::Token, engine::Command>> queue_select;

			extern std::atomic<int> entitytoggle;

			extern engine::graphics::renderer * self;
			extern engine::application::window * window;
			extern void (* callback_select)(engine::Token entity, engine::Command command, utility::any && data);
		}
	}
}

debug_assets("box", "cuboid", "dude", "my_png", "photo");

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
		const value_type & top() const { return stack.top(); }

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
	struct display_t
	{
		int x;
		int y;
		int width;
		int height;

		core::maths::Matrix4x4f projection_3d;
		core::maths::Matrix4x4f frame_3d;
		core::maths::Matrix4x4f view_3d;
		core::maths::Matrix4x4f inv_projection_3d;
		core::maths::Matrix4x4f inv_frame_3d;
		core::maths::Matrix4x4f inv_view_3d;

		core::maths::Matrix4x4f projection_2d;
		core::maths::Matrix4x4f view_2d;

		core::maths::Vector2f from_world_to_frame(core::maths::Vector3f wpos) const
		{
			return to_xy(frame_3d * projection_3d * view_3d * to_xyz1(wpos));
		}
	};

	core::container::Collection
	<
		engine::Token,
		utility::static_storage_traits<23>,
		utility::static_storage<10, display_t>
	>
	displays;

	struct update_display_camera_2d
	{
		engine::graphics::data::camera_2d && data;

		void operator () (display_t & x)
		{
			x.projection_2d = data.projection;
			x.view_2d = data.view;
		}
	};

	struct update_display_camera_3d
	{
		engine::graphics::data::camera_3d && data;

		void operator () (display_t & x)
		{
			x.projection_3d = data.projection;
			x.frame_3d = data.frame;
			x.view_3d = data.view;
			x.inv_projection_3d = data.inv_projection;
			x.inv_frame_3d = data.inv_frame;
			x.inv_view_3d = data.inv_view;
		}
	};

	struct update_display_viewport
	{
		engine::graphics::data::viewport && data;

		void operator () (display_t & x)
		{
			x.x = data.x;
			x.y = data.y;
			x.width = data.width;
			x.height = data.height;
		}
	};


	struct mesh_t
	{
		core::maths::Matrix4x4f modelview;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
		core::container::Buffer coords;
		std::vector<engine::model::weight_t> weights;

		explicit mesh_t(engine::graphics::data::MeshAsset && data)
			: modelview(core::maths::Matrix4x4f::identity())
			, vertices(std::move(data.vertices))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.coords))
		{}

		explicit mesh_t(engine::model::mesh_t && data)
			: modelview(data.matrix)
			, vertices(std::move(data.xyz))
			, triangles(std::move(data.triangles))
			, normals(std::move(data.normals))
			, coords(std::move(data.uv))
			, weights(std::move(data.weights))
		{}
	};

	struct ColorClass
	{
		engine::graphics::opengl::Color4ub diffuse;

		explicit ColorClass(uint32_t diffuse)
			: diffuse(diffuse >>  0 & 0x000000ff,
			          diffuse >>  8 & 0x000000ff,
			          diffuse >> 16 & 0x000000ff,
			          diffuse >> 24 & 0x000000ff)
		{}
	};

	struct DefaultClass
	{};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<401>,
		utility::static_storage<100, mesh_t>,
		utility::static_storage<100, ColorClass>,
		utility::static_storage<100, DefaultClass>
	>
	resources;


	struct ColorMaterial
	{
		engine::graphics::opengl::Color4ub diffuse;

		engine::Token materialclass;

		explicit ColorMaterial(uint32_t diffuse, engine::Token materialclass)
			: diffuse(diffuse >> 0 & 0x000000ff,
			          diffuse >> 8 & 0x000000ff,
			          diffuse >> 16 & 0x000000ff,
			          diffuse >> 24 & 0x000000ff)
			, materialclass(materialclass)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<201>,
		utility::static_storage<100, ColorMaterial>
	>
	materials;


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
		engine::Token,
		utility::static_storage_traits<1601>,
		utility::static_storage<600, object_modelview>,
		utility::static_storage<200, object_modelview_vertices>
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


	struct Character
	{
		const mesh_t * mesh_;
		object_modelview_vertices * object_;

		Character(
			const mesh_t & mesh,
			object_modelview_vertices & object)
			: mesh_(&mesh)
			, object_(&object)
		{}

		void draw(const bool highlighted)
		{
			const mesh_t & mesh = *mesh_;

			if (highlighted)
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(
					3, // TODO
				    GL_FLOAT, // TODO
				    0,
				    object_->vertices.data());
				glNormalPointer(
					GL_FLOAT, // TODO
				    0,
				    mesh.normals.data());
				glDrawElements(
					GL_TRIANGLES,
					debug_cast<GLsizei>(mesh.triangles.count()),
					GL_UNSIGNED_SHORT,
					mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			else
			{
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(
					3, // TODO
					GL_FLOAT, // TODO
					0,
					object_->vertices.data());
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
					debug_cast<GLsizei>(mesh.triangles.count()),
					GL_UNSIGNED_SHORT, // TODO
					mesh.triangles.data());
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
	};

	struct MeshObject
	{
		const object_modelview * object;
		const mesh_t * mesh;
		engine::Token material;
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<Character>,
		utility::heap_storage<MeshObject>
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

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<selectable_character_t>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		void operator () (engine::Token entity, Character & x)
		{
			debug_verify(selectable_components.emplace<selectable_character_t>(entity, x.mesh_, x.object_, color));
		}

		template <typename T>
		void operator () (engine::Token /*entity*/, T &)
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
			if (matrix_pallet.empty())
				return;

			const float * const untransformed_vertices = mesh->vertices.data_as<float>();
			float * const transformed_vertices = object->vertices.data_as<float>();
			for (std::ptrdiff_t i : ranges::index_sequence(object->vertices.count() / 3))
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
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<updateable_character_t>
	>
	updateable_components;

	struct update_matrixpallet
	{
		engine::graphics::data::CharacterSkinning && data;

		void operator () ()
		{
		}
		void operator () (updateable_character_t & x)
		{
			x.matrix_pallet = std::move(data.matrix_pallet);
		}
	};


	struct highlighted_t
	{
	};

	struct selected_t
	{
	};

	core::container::MultiCollection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<highlighted_t>,
		utility::heap_storage<selected_t>
	>
	selected_components;
}

namespace
{
	using namespace engine::graphics::detail;

	void maybe_resize_framebuffer();
	void poll_queues()
	{
		bool should_maybe_resize_framebuffer = false;

		Message message;
		while (message_queue.try_pop(message))
		{
			struct ProcessMessage
			{
				bool & should_maybe_resize_framebuffer;

				void operator () (MessageAddDisplay && x)
				{
					debug_verify(displays.emplace<display_t>(
						x.asset,
						x.display.viewport.x, x.display.viewport.y, x.display.viewport.width, x.display.viewport.height,
						x.display.camera_3d.projection, x.display.camera_3d.frame, x.display.camera_3d.view, x.display.camera_3d.inv_projection, x.display.camera_3d.inv_frame, x.display.camera_3d.inv_view,
						x.display.camera_2d.projection, x.display.camera_2d.view));
					should_maybe_resize_framebuffer = true;
				}

				void operator () (MessageRemoveDisplay && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.erase(display_it);
						should_maybe_resize_framebuffer = true;
					}
				}

				void operator () (MessageUpdateDisplayCamera2D && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_camera_2d{std::move(x.camera_2d)});
					}
				}

				void operator () (MessageUpdateDisplayCamera3D && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_camera_3d{std::move(x.camera_3d)});
					}
				}

				void operator () (MessageUpdateDisplayViewport && x)
				{
					const auto display_it = find(displays, x.asset);
					if (debug_assert(display_it != displays.end()))
					{
						displays.call(display_it, update_display_viewport{std::move(x.viewport)});
						should_maybe_resize_framebuffer = true;
					}
				}

				void operator () (MessageRegisterCharacter && x)
				{
					debug_verify(resources.emplace<mesh_t>(x.asset, std::move(x.mesh)));
				}

				void operator () (MessageRegisterMaterial && x)
				{
					if (x.material.data_opengl_12.diffuse)
					{
						debug_verify(resources.replace<ColorClass>(x.asset, x.material.data_opengl_12.diffuse.value()));
					}
					else
					{
						debug_verify(resources.replace<DefaultClass>(x.asset));
					}
				}

				void operator () (MessageRegisterMesh && x)
				{
					debug_verify(resources.replace<mesh_t>(x.asset, std::move(x.mesh)));
				}

				void operator () (MessageRegisterTexture && /*x*/)
				{
					debug_fail("missing implementation");
				}

				void operator () (MessageCreateMaterialInstance && x)
				{
					if (!debug_verify(find(resources, x.data.materialclass) != resources.end(), x.data.materialclass))
						return; // error

					const auto material_it = find(materials, x.entity);
					if (material_it != materials.end())
					{
						if (!debug_assert(materials.get_key(material_it) < x.entity, "trying to add an older version object"))
							return; // error

						materials.erase(material_it);
					}
					debug_verify(materials.emplace<ColorMaterial>(x.entity, x.data.diffuse, x.data.materialclass));
				}

				void operator () (MessageDestroy && x)
				{
					const auto material_it = find(materials, x.entity);
					if (debug_assert(material_it != materials.end()))
					{
						materials.erase(material_it);
					}
				}

				void operator () (MessageAddMeshObject && x)
				{
					const auto object_it = find(objects, x.entity);
					if (object_it != objects.end())
					{
						if (!debug_assert(objects.get_key(object_it) < x.entity, "trying to add an older version object"))
							return; // error

						objects.erase(object_it);
					}
					auto * const object = objects.emplace<object_modelview>(x.entity, std::move(x.object.matrix));

					const auto mesh_it = find(resources, x.object.mesh);
					if (!debug_verify(mesh_it != resources.end(), "missing mesh"))
						return; // error

					if (!debug_verify(resources.contains<mesh_t>(mesh_it), "invalid mesh"))
						return; // error

					mesh_t * const mesh = resources.get<mesh_t>(mesh_it);

					const auto component_it = find(components, x.entity);
					if (component_it != components.end())
					{
						if (!debug_assert(components.get_key(component_it) < x.entity, "trying to add an older version mesh"))
							return; // error

						components.erase(component_it);
					}
					debug_verify(components.emplace<MeshObject>(x.entity, object, mesh, x.object.material));
				}

				void operator () (MessageMakeObstruction && x)
				{
					const engine::graphics::opengl::Color4ub color = {0, 0, 0, 0};

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.call(selectable_it, set_selectable_color{color});
					}
					else
					{
						const auto component_it = find(components, x.entity);
						if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
							return; // error

						components.call(component_it, add_selectable_color{color});
					}
				}

				void operator () (MessageMakeSelectable && x)
				{
					const engine::graphics::opengl::Color4ub color = {(x.entity & 0x000000ff) >> 0,
					                                                  (x.entity & 0x0000ff00) >> 8,
					                                                  (x.entity & 0x00ff0000) >> 16,
					                                                  (x.entity & 0xff000000) >> 24};

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.call(selectable_it, set_selectable_color{color});
					}
					else
					{
						const auto component_it = find(components, x.entity);
						if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
							return; // error

						components.call(component_it, add_selectable_color{color});
					}
				}

				void operator () (MessageMakeTransparent && x)
				{
					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.erase(selectable_it);
					}
				}

				void operator () (MessageMakeClearSelection &&)
				{
					debug_fail(); // not implemented yet
				}

				void operator () (MessageMakeDehighlighted && x)
				{
					const auto selected_it = find(selected_components, x.entity);
					if (selected_it != selected_components.end())
					{
						if (selected_components.contains<highlighted_t>(selected_it))
						{
							selected_components.erase<highlighted_t>(selected_it);
						}
					}
				}

				void operator () (MessageMakeDeselect && x)
				{
					const auto selected_it = find(selected_components, x.entity);
					if (selected_it != selected_components.end())
					{
						if (selected_components.contains<selected_t>(selected_it))
						{
							selected_components.erase<selected_t>(selected_it);
						}
					}
				}

				void operator () (MessageMakeHighlighted && x)
				{
					debug_verify(selected_components.emplace<highlighted_t>(x.entity));
				}

				void operator () (MessageMakeSelect && x)
				{
					debug_verify(selected_components.emplace<selected_t>(x.entity));
				}

				void operator () (MessageRemove && x)
				{
					const auto component_it = find(components, x.entity);
					if (!debug_assert(component_it != components.end(), "unknown component ", x.entity))
						return; // error

					const auto selectable_it = find(selectable_components, x.entity);
					if (selectable_it != selectable_components.end())
					{
						selectable_components.erase(selectable_it);
					}

					const auto updateable_it = find(updateable_components, x.entity);
					if (updateable_it != updateable_components.end())
					{
						updateable_components.erase(updateable_it);
					}

					components.erase(component_it);

					const auto object_it = find(objects, x.entity);
					if (debug_assert(object_it != objects.end()))
					{
						objects.erase(object_it);
					}
				}

				void operator () (MessageUpdateCharacterSkinning && x)
				{
					const auto updateable_it = find(updateable_components, x.entity);
					if (debug_assert(updateable_it != updateable_components.end()))
					{
						updateable_components.call(updateable_it, update_matrixpallet{std::move(x.character_skinning)});
					}
				}

				void operator () (MessageUpdateModelviewMatrix && x)
				{
					const auto object_it = find(objects, x.entity);
					if (debug_assert(object_it != objects.end()))
					{
						objects.call(object_it, update_modelview{std::move(x.modelview_matrix)});
					}
				}
			};
			visit(ProcessMessage{should_maybe_resize_framebuffer}, std::move(message));
		}
		if (should_maybe_resize_framebuffer)
		{
			maybe_resize_framebuffer();
		}
	}
}

namespace
{
	Stack modelview_matrix;

	int framebuffer_width = 0;
	int framebuffer_height = 0;
	GLuint framebuffer;
	GLuint entitybuffers[2]; // color, depth
	std::vector<uint32_t> entitypixels;
	engine::graphics::opengl::Color4ub highlighted_color{255, 191, 64, 255};
	engine::graphics::opengl::Color4ub selected_color{64, 191, 255, 255};

	// todo remove
	void resize_framebuffer(int width, int height)
	{
		framebuffer_width = width;
		framebuffer_height = height;

		// free old render buffers
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0); // color
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0); // depth
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		glDeleteRenderbuffers(2, entitybuffers);
		// allocate new render buffers
		glGenRenderbuffers(2, entitybuffers);
		glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[0]); // color
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, framebuffer_width, framebuffer_height);
		glBindRenderbuffer(GL_RENDERBUFFER, entitybuffers[1]); // depth
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, framebuffer_width, framebuffer_height);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, entitybuffers[0]); // color
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, entitybuffers[1]); // depth
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		entitypixels.resize(framebuffer_width * framebuffer_height);
	}
	void maybe_resize_framebuffer()
	{
		int width = 0;
		int height = 0;

		for (const auto & display : displays.get<display_t>())
		{
			const int maybe_width = display.x + display.width;
			if (maybe_width > width) width = maybe_width;
			const int maybe_height = display.y + display.height;
			if (maybe_height > height) height = maybe_height;
		}

		if (width != framebuffer_width ||
		    height != framebuffer_height)
		{
			resize_framebuffer(width, height);
		}
	}

	engine::Token get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = framebuffer_height - 1 - sy;

		if (x >= 0 &&
		    y >= 0 &&
		    x < 0 + framebuffer_width &&
		    y < 0 + framebuffer_height)
		{
			const unsigned int color = entitypixels[x + y * framebuffer_width];
			return engine::Token(
				(color & 0xff000000) >> 24 |
				(color & 0x00ff0000) >> 8 |
				(color & 0x0000ff00) << 8 |
				(color & 0x000000ff) << 24);
		}
		return engine::Token{};
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

	void render_setup()
	{
		debug_printline(engine::graphics_channel, "render_callback starting");
		make_current(*engine::graphics::detail::window);

		debug_printline(engine::graphics_channel, "glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		debug_printline(engine::graphics_channel, "glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		debug_printline(engine::graphics_channel, "glGetString GL_VERSION: ", glGetString(GL_VERSION));

		engine::graphics::opengl::init();

		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		initLights();

		// entity buffers
		glGenFramebuffers(1, &framebuffer);
		glGenRenderbuffers(2, entitybuffers);
	}

	constexpr std::array<GLenum, 10> BufferFormats =
	{{
		GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, (GLenum)-1, GL_BYTE, GL_SHORT, GL_INT, (GLenum)-1, GL_FLOAT, GL_DOUBLE
	}};

	void render_update()
	{
		profile_scope("renderer update (opengl 1.2)");

		// poll events
		poll_queues();
		// update components
		for (auto & component : updateable_components.get<updateable_character_t>())
		{
			component.update();
		}

		if (framebuffer_width == 0 || framebuffer_height == 0)
			return;

		glStencilMask(0x000000ff);
		// setup frame
		glClearColor(0.f, 0.f, .1f, 0.f);
		glClearDepth(1.);
		//glClearStencil(0x00000000);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		////////////////////////////////////////////////////////
		//
		//  entity buffer
		//
		////////////////////////////////////////
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearColor(0.f, 0.f, 0.f, 0.f); // null entity
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);

		for (const auto & display : displays.get<display_t>())
		{
			glViewport(display.x, display.y, display.width, display.height);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_3d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_3d);

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
			               debug_cast<GLsizei>(component.mesh->triangles.count()),
			               GL_UNSIGNED_SHORT,	// TODO
			               component.mesh->triangles.data());
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();
		}

		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_2d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_2d);

		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

		glReadPixels(0, 0, framebuffer_width, framebuffer_height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, entitypixels.data());

		// select
		{
			std::tuple<int, int, engine::Token, engine::Command> select_args;
			while (queue_select.try_pop(select_args))
			{
				engine::graphics::data::SelectData select_data = {get_entity_at_screen(std::get<0>(select_args), std::get<1>(select_args)), {std::get<0>(select_args), std::get<1>(select_args)}};
				callback_select(std::get<2>(select_args), std::get<3>(select_args), std::move(select_data));
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		//////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////

		for (const auto & display : displays.get<display_t>())
		{
			glViewport(display.x, display.y, display.width, display.height);

		// setup 3D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_3d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_3d);

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
			modelview_matrix.mult(component.object_->modelview);
			modelview_matrix.mult(component.mesh_->modelview);
			glLoadMatrix(modelview_matrix);

			const auto entity = components.get_key(component);
			const auto selected_it = find(selected_components, entity);
			const bool is_highlighted = selected_it != selected_components.end() && selected_components.contains<highlighted_t>(selected_it);
			const bool is_selected = selected_it != selected_components.end() && selected_components.contains<selected_t>(selected_it);

			if (is_highlighted)
				glColor(highlighted_color);
			else if (is_selected)
				glColor(selected_color);

			component.draw(is_highlighted || is_selected);

			modelview_matrix.pop();

			debug_assert(glGetError() == GL_NO_ERROR);
		}

		for (auto & component : components.get<MeshObject>())
		{
			engine::graphics::opengl::Color4ub color(0, 0, 0, 0);

			const auto material_it = find(materials, component.material);
			if (material_it != materials.end())
			{
				if (auto * const material = materials.get<ColorMaterial>(material_it))
				{
					color = material->diffuse;

					const auto resource_it = find(resources, material->materialclass);
					if (resource_it != resources.end())
					{
						if (auto * const color_class = resources.get<ColorClass>(resource_it))
						{
							color = color_class->diffuse;
						}
					}
				}
			}

			modelview_matrix.push();
			modelview_matrix.mult(component.object->modelview);
			modelview_matrix.mult(component.mesh->modelview);
			glLoadMatrix(modelview_matrix);

			glColor(color);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);

			glVertexPointer(
				3, // todo support 2d coordinates?
				BufferFormats[static_cast<int>(component.mesh->vertices.format())],
				0,
				component.mesh->vertices.data());
			glNormalPointer(
				BufferFormats[static_cast<int>(component.mesh->normals.format())],
				0,
				component.mesh->normals.data());
			glDrawElements(
				GL_TRIANGLES,
				debug_cast<GLsizei>(component.mesh->triangles.count()),
				BufferFormats[static_cast<int>(component.mesh->triangles.format())],
				component.mesh->triangles.data());

			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			modelview_matrix.pop();

			debug_assert(glGetError() == GL_NO_ERROR);
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);


		// setup 2D
		glMatrixMode(GL_PROJECTION);
		glLoadMatrix(display.projection_2d);
		glMatrixMode(GL_MODELVIEW);
		modelview_matrix.load(display.view_2d);

		// disable depth test to make the bar drawings show above the 3d content
		glDisable(GL_DEPTH_TEST);

		// 2d
		// ...

		// clear depth to make GUI show over all prev. rendering
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);

		// draw gui
		// ...

		}

		// entity buffers
		if (entitytoggle.load(std::memory_order_relaxed))
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

			glBlitFramebuffer(0, 0, framebuffer_width, framebuffer_height, 0, 0, framebuffer_width, framebuffer_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		// swap buffers
		swap_buffers(*engine::graphics::detail::window);
	}

	void render_teardown()
	{
		debug_printline(engine::graphics_channel, "render_callback stopping");

		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);

		engine::Token resources_not_unregistered[resources.max_size()];
		const auto resource_count = resources.get_all_keys(resources_not_unregistered, resources.max_size());
		debug_printline(engine::asset_channel, resource_count, " resources not unregistered:");
		for (auto i : ranges::index_sequence(resource_count))
		{
			debug_printline(engine::asset_channel, resources_not_unregistered[i]);
			static_cast<void>(i);
		}

		engine::Token materials_not_destroyed[materials.max_size()];
		const auto material_count = materials.get_all_keys(materials_not_destroyed, materials.max_size());
		debug_printline(engine::asset_channel, material_count, " materials not destroyed:");
		for (auto i : ranges::index_sequence(material_count))
		{
			debug_printline(engine::asset_channel, materials_not_destroyed[i]);
			static_cast<void>(i);
		}
	}
}

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			namespace opengl_12
			{
				void run()
				{
					render_setup();

					event.wait();
					event.reset();

					while (active.load(std::memory_order_relaxed))
					{
						render_update();

						event.wait();
						event.reset();
					}

					render_teardown();
				}
			}
		}
	}
}
