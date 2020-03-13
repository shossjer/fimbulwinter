
#include "renderer.hpp"

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
#include "engine/graphics/viewer.hpp"

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

			extern core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			extern std::atomic<int> entitytoggle;

			extern engine::graphics::renderer * self;
			extern engine::application::window * window;
			extern void (* callback_select)(engine::Entity entity, engine::Command command, utility::any && data);
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
		engine::Asset,
		21,
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

			switch (image.color())
			{
			case core::graphics::ColorType::RGB:
				glTexImage2D(GL_TEXTURE_2D, 0, 3, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
				break;
			case core::graphics::ColorType::RGBA:
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
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
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


	struct Character
	{
		const mesh_t * mesh_;
		const texture_t * texture_;
		object_modelview_vertices * object_;

		Character(const mesh_t & mesh, const texture_t & texture, object_modelview_vertices & object)
			: mesh_(&mesh)
			, texture_(&texture)
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
				texture_->enable();

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

				texture_->disable();
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		1601,
		utility::heap_storage<Character>
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
		engine::Entity,
		1601,
		utility::heap_storage<selectable_character_t>
	>
	selectable_components;

	struct add_selectable_color
	{
		engine::graphics::opengl::Color4ub color;

		void operator () (engine::Entity entity, Character & x)
		{
			debug_verify(selectable_components.try_emplace<selectable_character_t>(entity, x.mesh_, x.object_, color));
		}

		template <typename T>
		void operator () (engine::Entity /*entity*/, T &)
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
		engine::Entity,
		1601,
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
		engine::Entity,
		201,
		std::array<highlighted_t, 100>,
		std::array<selected_t, 100>
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
					debug_verify(displays.try_emplace<display_t>(
						x.asset,
						x.display.viewport.x, x.display.viewport.y, x.display.viewport.width, x.display.viewport.height,
						x.display.camera_3d.projection, x.display.camera_3d.frame, x.display.camera_3d.view, x.display.camera_3d.inv_projection, x.display.camera_3d.inv_frame, x.display.camera_3d.inv_view,
						x.display.camera_2d.projection, x.display.camera_2d.view));
					should_maybe_resize_framebuffer = true;
				}

				void operator () (MessageRemoveDisplay && x)
				{
					displays.remove(x.asset);
					should_maybe_resize_framebuffer = true;
				}

				void operator () (MessageUpdateDisplayCamera2D && x)
				{
					displays.call(x.asset, update_display_camera_2d{std::move(x.camera_2d)});
				}

				void operator () (MessageUpdateDisplayCamera3D && x)
				{
					displays.call(x.asset, update_display_camera_3d{std::move(x.camera_3d)});
				}

				void operator () (MessageUpdateDisplayViewport && x)
				{
					displays.call(x.asset, update_display_viewport{std::move(x.viewport)});
					should_maybe_resize_framebuffer = true;
				}

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

				void operator () (MessageMakeObstruction && x)
				{
					const engine::graphics::opengl::Color4ub color = {0, 0, 0, 0};
					if (selectable_components.contains(x.entity))
					{
						selectable_components.call(x.entity, set_selectable_color{color});
					}
					else
					{
						components.call(x.entity, add_selectable_color{color});
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
						components.call(x.entity, add_selectable_color{color});
					}
				}

				void operator () (MessageMakeTransparent && x)
				{
					if (selectable_components.contains(x.entity))
					{
						selectable_components.remove(x.entity);
					}
				}

				void operator () (MessageMakeClearSelection &&)
				{
					debug_fail(); // not implemented yet
				}

				void operator () (MessageMakeDehighlighted && x)
				{
					selected_components.try_remove<highlighted_t>(x.entity);
				}

				void operator () (MessageMakeDeselect && x)
				{
					selected_components.try_remove<selected_t>(x.entity);
				}

				void operator () (MessageMakeHighlighted && x)
				{
					selected_components.emplace<highlighted_t>(x.entity);
				}

				void operator () (MessageMakeSelect && x)
				{
					selected_components.emplace<selected_t>(x.entity);
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

	engine::graphics::opengl::Font normal_font;

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

	engine::Entity get_entity_at_screen(int sx, int sy)
	{
		const int x = sx;
		const int y = framebuffer_height - 1 - sy;

		if (x >= 0 &&
		    y >= 0 &&
		    x < 0 + framebuffer_width &&
		    y < 0 + framebuffer_height)
		{
			const unsigned int color = entitypixels[x + y * framebuffer_width];
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
			std::tuple<int, int, engine::Entity, engine::Command> select_args;
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
			const bool is_highlighted = selected_components.contains<highlighted_t>(entity);
			const bool is_selected = selected_components.contains<selected_t>(entity);

			if (is_highlighted)
				glColor(highlighted_color);
			else if (is_selected)
				glColor(selected_color);

			component.draw(is_highlighted || is_selected);

			modelview_matrix.pop();
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
		glLoadMatrix(modelview_matrix);
		glColor3ub(255, 255, 0);
		normal_font.draw(10, 10 + 12, "herp derp herp derp herp derp herp derp herp derp etc.");

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
		// vvvvvvvv tmp vvvvvvvv
		{
			normal_font.decompile();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
		glDeleteRenderbuffers(2, entitybuffers);
		glDeleteFramebuffers(1, &framebuffer);

		engine::Asset resources_not_unregistered[resources.max_size()];
		const int resource_count = resources.get_all_keys(resources_not_unregistered, resources.max_size());
		debug_printline(engine::asset_channel, resource_count, " resources not unregistered:");
		for (int i = 0; i < resource_count; i++)
		{
			debug_printline(engine::asset_channel, resources_not_unregistered[i]);
		}

		engine::Asset materials_not_unregistered[materials.max_size()];
		const int material_count = materials.get_all_keys(materials_not_unregistered, materials.max_size());
		debug_printline(engine::asset_channel, material_count, " materials not unregistered:");
		for (int i = 0; i < material_count; i++)
		{
			debug_printline(engine::asset_channel, materials_not_unregistered[i]);
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
