
#include "renderer.hpp"

#include "events.hpp"
#include "opengl.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>
#include <core/color.hpp>
#include <core/container/Stack.hpp>
#include <core/maths/Matrix.hpp>
#include <engine/debug.hpp>
#include <utility/type_traits.hpp>

#include <atomic>
#include <cstring> // memset
#include <stdexcept>
#include <tuple>
#include <utility>

namespace engine
{
	namespace physics
	{
		extern void setup();
		extern void update();
		extern void render();
	}

	namespace application
	{
		namespace window
		{
			extern void make_current();
			extern void swap_buffers();
		}
	}
	namespace graphics
	{
		extern void poll_messages();
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
			debug_assert(this->stack.size() > 1);
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
	struct dimension_t
	{
		int32_t width, height;
	};

	std::atomic_bool active;
	core::async::Thread render_thread;

	dimension_t resizes[3];
	bool masks[3] = {false, false, false};
	int read_resize = 0;
	int write_resize = 1;
	std::atomic_int latest_resize{2};

	/** Current window dimensions */
	dimension_t dimension = {100, 100}; // initialized to something positive

	core::maths::Matrix4x4f projection2D;
	core::maths::Matrix4x4f projection3D;

	core::maths::Matrix4x4f view3D;
	// core::container::Stack<core::maths::Matrix4x4f, 10> modelview_stack;
	Stack modelview_matrix;

	engine::graphics::opengl::Font normal_font;

	void set(engine::graphics::Point & component, const engine::graphics::ModelMatrixMessage & message)
	{
		component.matrix = message.model_matrix;
	}
	template <typename T, typename M>
	void set(T & component, const M & message)
	{
		debug_unreachable();
	}

	template <std::size_t M, typename ...Arrays>
	class HashCollection;
	template <std::size_t M, typename ...Ts, std::size_t ...Ns>
	class HashCollection<M, std::array<Ts, Ns>...>
	{
	public:
		template <typename T, std::size_t N>
		struct Collection
		{
			static constexpr int capacity = N;

			std::size_t size;
			std::size_t objects[N];
			T components[N];

			const T * begin() const
			{
				return this->components;
			}
			const T * end() const
			{
				return this->components + size;
			}
		};

	public:
		static constexpr int capacity = M;
		static constexpr int ncollections = sizeof...(Ts); // or Ns

	private:
		std::size_t indices[capacity];
		std::tuple<Collection<Ts, Ns>...> collections;

	public:
		HashCollection()
		{
			std::memset(this->indices, 0xff, this->capacity * sizeof(std::size_t));
		}

	public:
		template <typename T, std::size_t I = mpl::index_of<mpl::decay_t<T>,
		                                                    mpl::type_list<Ts...>>::value>
		auto get() const -> decltype(std::get<I>(this->collections))
		{
			return std::get<I>(this->collections);
		}

		/**
		 * Takes O(1)
		 */
		template <typename T>
		void add(const std::size_t object, T && component)
		{
			constexpr std::size_t I = mpl::index_of<mpl::decay_t<T>,
			                                        mpl::type_list<Ts...>>::value;
			auto & collection = std::get<I>(this->collections);
			debug_assert(collection.size < collection.capacity);

			const std::size_t bucket = hash(object);
			debug_assert(indices[bucket] == std::size_t(-1));

			constexpr std::size_t id = I;
			const     std::size_t ix = collection.size; // 0x00ffffff & collection.size

			indices[bucket] = (id << 24) | ix;
			collection.components[ix] = std::forward<T>(component);
			collection.objects[ix] = object;
			collection.size++;
		}
		/**
		 * Takes O(n), where n is the number of collections.
		 */
		void remove(const std::size_t object)
		{
			const std::size_t bucket = hash(object);
			const std::size_t index = indices[bucket];
			debug_assert(index != std::size_t(-1));

			indices[bucket] = std::size_t(-1);
			this->remove(mpl::index_constant<0>{}, index >> 24, index);
		}

		/**
		 * Takes O(n), where n is the number of collections.
		 */
		template <typename Msg>
		void set(const Msg & message)
		{
			const std::size_t bucket = hash(message.object);
			const std::size_t index = indices[bucket];
			debug_assert(index != std::size_t(-1));

			this->set(mpl::index_constant<0>{}, index >> 24, index, message);
		}
	private:
		void remove(mpl::index_constant<ncollections> /**/, const std::size_t id, const std::size_t index)
		{
			debug_unreachable();
		}
		template <std::size_t I>
		void remove(mpl::index_constant<I> /**/, const std::size_t id, const std::size_t index)
		{
			if (id == I)
			{
				auto & collection = std::get<I>(this->collections);

				const std::size_t ix = 0x00ffffff & index;
				const std::size_t last = collection.size - 1;

				if (ix < last)
				{
					indices[hash(collection.objects[last])] = index;
					collection.components[ix] = collection.components[last];
					collection.objects[ix] = collection.objects[last];
				}
				collection.size--;
			}
			else
			{
				this->remove(mpl::index_constant<I + 1>{}, id, index);
			}
		}

		template <typename Msg>
		void set(mpl::index_constant<ncollections> /**/, const std::size_t id, const std::size_t index, const Msg & message)
		{
			debug_unreachable();
		}
		template <std::size_t I, typename Msg>
		void set(mpl::index_constant<I> /**/, const std::size_t id, const std::size_t index, const Msg & message)
		{
			if (id == I)
			{
				auto & collection = std::get<I>(this->collections);

				const std::size_t ix = 0x00ffffff & index;

				::set(collection.components[ix], message);
			}
			else
			{
				this->set(mpl::index_constant<I + 1>{}, id, index, message);
			}
		}

		std::size_t hash(const std::size_t object) const
		{
			return object % this->capacity;
		}

	};

	HashCollection<1000,
	               std::array<engine::graphics::Point, 100>,
	               std::array<engine::graphics::Rectangle, 100>> collection;

	void initLights()
	{
		// set up light colors (ambient, diffuse, specular)
		GLfloat lightKa[] = { .2f, .2f, .2f, 1.0f };  // ambient light
		GLfloat lightKd[] = { .7f, .7f, .7f, 1.0f };  // diffuse light
		GLfloat lightKs[] = { 1, 1, 1, 1 };           // specular light
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

		// position the light
		float lightPos[4] = { 0, 0, 20, 1 }; // positional light
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
	}

	void render_callback()
	{
		graphics_debug_trace("render_callback starting");
		engine::application::window::make_current();

		graphics_debug_trace("glGetString GL_VENDOR: ", glGetString(GL_VENDOR));
		graphics_debug_trace("glGetString GL_RENDERER: ", glGetString(GL_RENDERER));
		graphics_debug_trace("glGetString GL_VERSION: ", glGetString(GL_VERSION));
#if WINDOW_USE_X11
		graphics_debug_trace("glGetString GL_SHADING_LANGUAGE_VERSION: ", glGetString(GL_SHADING_LANGUAGE_VERSION));
#endif

		glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment

													// enable /disable features
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		//glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);

		// track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		//	glClearColor(0, 0, 0, 0);                   // background color
		glClearStencil(0);                          // clear stencil buffer
		glClearDepth(1.0f);                         // 0 is near, 1 is far
		glDepthFunc(GL_LEQUAL);

		initLights();

		// vvvvvvvv tmp vvvvvvvv
		{
			engine::graphics::opengl::Font::Data data;

			if (!data.load("consolas", 12))
			{
				debug_assert(("COULD NOT LOAD THE FONT: maybe it is missing?", false));
			}
			normal_font.compile(data);

			data.free();
		}
		// ^^^^^^^^ tmp ^^^^^^^^

		engine::physics::setup();

		while (active)
		{
			// handle notifications
			read_resize = latest_resize.exchange(read_resize); // std::memory_order_acquire?
			if (masks[read_resize])
			{
				dimension = resizes[read_resize];
				masks[read_resize] = false;

				glViewport(0, 0, dimension.width, dimension.height);

				// these calculations do not need opengl context
				projection2D = core::maths::Matrix4x4f::ortho(0.f, dimension.width, dimension.height, 0.f, -1.f, 1.f);
				projection3D = core::maths::Matrix4x4f::perspective(core::maths::make_degree(80.f), float(dimension.width) / float(dimension.height), .125f, 128.f);
			}
			//
			engine::graphics::poll_messages();
			// setup frame
			static core::color::hsv_t<float> tmp1{0, 1, 1};
			tmp1 += core::color::hue_t<float>{1};
			const auto tmp2 = make_rgb(tmp1);
			// glClearColor(tmp2.red(), tmp2.green(), tmp2.blue(), 0.f);
			glClearColor(0.f, 0.f, .1f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// setup 3D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection3D);
			glMatrixMode(GL_MODELVIEW);
			// vvvvvvvv tmp vvvvvvvv
			view3D = core::maths::Matrix4x4f::translation(0.f, -1.f, -20.f);
			// ^^^^^^^^ tmp ^^^^^^^^
			modelview_matrix.load(view3D);

			glEnable(GL_DEPTH_TEST);

			// 3d
			for (auto && point : collection.get<engine::graphics::Point>())
			{
				glPointSize(point.size);

				modelview_matrix.push();
				modelview_matrix.mult(point.matrix);
				glLoadMatrix(modelview_matrix);

				glColor3f(tmp2.red(), tmp2.green(), tmp2.blue());
				glBegin(GL_POINTS);
				glVertex3f(0.f, 0.f, 0.f);
				glEnd();

				modelview_matrix.pop();
			}

			glLoadMatrix(modelview_matrix);
			// tmp rotating thing
			static double deg = 0.;
			if ((deg += 1.) >= 360.) deg -= 360.;
			glRotated(deg, 0.f, 1.f, 0.f);
			glBegin(GL_LINES);
			glColor3ub(255, 0, 0);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(100.f, 0.f, 0.f);
			glColor3ub(0, 255, 0);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 100.f, 0.f);
			glColor3ub(0, 0, 255);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 0.f, 100.f);
			glEnd();

			engine::physics::update();
			engine::physics::render();

			// setup 2D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection2D);
			glMatrixMode(GL_MODELVIEW);
			modelview_matrix.load(core::maths::Matrix4x4f::identity());

			// glDisable(GL_DEPTH_TEST);
			glEnable(GL_DEPTH_TEST);

			// draw gui
			// ...
			glLoadMatrix(modelview_matrix);
			glColor3ub(255, 255, 255);
			glRasterPos2i(10, 10 + 12);
			normal_font.draw("herp derp herp derp herp derp herp derp herp derp etc.");
			// 2d
			for (auto && rectangle : collection.get<engine::graphics::Rectangle>())
			{
				modelview_matrix.push();
				modelview_matrix.translate(dimension.width / 2.f, dimension.height / 2.f, rectangle.z);
				glLoadMatrix(modelview_matrix);

				glColor3f(rectangle.red, rectangle.green, rectangle.blue);
				glBegin(GL_QUADS);
				glVertex3f(rectangle.x                  , rectangle.y                   , 0.f);
				glVertex3f(rectangle.x                  , rectangle.y + rectangle.height, 0.f);
				glVertex3f(rectangle.x + rectangle.width, rectangle.y + rectangle.height, 0.f);
				glVertex3f(rectangle.x + rectangle.width, rectangle.y                   , 0.f);
				glEnd();

				modelview_matrix.pop();
			}

			// something temporary that delays
			core::async::delay(10);

			// swap buffers
			engine::application::window::swap_buffers();
		}
		graphics_debug_trace("render_callback stopping");
		// vvvvvvvv tmp vvvvvvvv
		{
			normal_font.decompile();
		}
		// ^^^^^^^^ tmp ^^^^^^^^
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
				active = true;

				core::async::Thread render_thread{render_callback};

				::render_thread = std::move(render_thread);
			}
			void destroy()
			{
				active = false;

				render_thread.join();
			}

			template <typename T>
			void add(const std::size_t object, const T & component)
			{
				collection.add(object, component);
			}
			void remove(const std::size_t object)
			{
				collection.remove(object);
			}

			template <typename Msg>
			void set(const Msg & message)
			{
				collection.set(message);
			}

			void notify_resize(const int width, const int height)
			{
				resizes[write_resize].width = width;
				resizes[write_resize].height = height;
				masks[write_resize] = true;
				write_resize = latest_resize.exchange(write_resize); // std::memory_order_release?
			}

			template void add<Point>(const std::size_t object, const Point & component);
			template void add<Rectangle>(const std::size_t object, const Rectangle & component);

			template void set<ModelMatrixMessage>(const ModelMatrixMessage & message);
		}
	}
}
