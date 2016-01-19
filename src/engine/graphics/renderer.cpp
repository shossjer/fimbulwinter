
#include "renderer.hpp"

#include "events.hpp"
#include "opengl.hpp"
#include "opengl/Matrix.hpp"

#include <config.h>

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>
#include <core/color.hpp>
#include <core/maths/Matrix.hpp>
#include <engine/debug.hpp>

#include <atomic>
#include <cstring> // memset
#include <stdexcept>
#include <tuple>
#include <type_traits> // mpl
#include <utility>

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
	namespace graphics
	{
		extern void poll_messages();
	}
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

	/** Current 2D projection matrix */
	core::maths::Matrixf projection2D;
	/** Current 3D projection matrix */
	core::maths::Matrixf projection3D;
	/** Current 3D view/camera matrix */
	core::maths::Matrixf view3D;

	void set(engine::graphics::Point & component, const engine::graphics::ModelMatrixMessage & message)
	{
		component.matrix = message.model_matrix;
	}
	template <typename T, typename M>
	void set(T & component, const M & message)
	{
		debug_unreachable();
	}

	namespace mpl
	{
		template <typename T>
		using decay_t = typename std::decay<T>::type;

		template <std::size_t N>
		using index_constant = std::integral_constant<std::size_t, N>;


		template <typename T>
		struct type_is
		{
			using type = T;
		};

		template <typename ...Ts>
		struct list {};

		// template <std::size_t I, typename ...Ts>
		// struct select;
		// template <typename T, typename ...Ts>
		// struct select<0, T, Ts...> : type_is<T> {};
		// template <std::size_t I, typename T, typename ...Ts>
		// struct select<I, T, Ts...> : select<I - 1, Ts...> {};
		// template <std::size_t I, typename ...Ts>
		// using select_t = typename select<I, Ts...>::type;

		template <typename T, typename L>
		struct index_of;
		template <typename T, typename ...Ts>
		struct index_of<T, list<T, Ts...>> : index_constant<0> {};
		template <typename T, typename T1, typename ...Ts>
		struct index_of<T, list<T1, Ts...>> : index_constant<1 + index_of<T, list<Ts...>>::value> {};

		template <typename T, T ...Ns>
		struct integral_sequence {};

		template <typename T, typename S, T ...Ns>
		struct append;
		template <typename T, T ...Ms, T ...Ns>
		struct append<T, integral_sequence<T, Ms...>, Ns...> : type_is<integral_sequence<T, Ns..., Ms...>> {};
		template <typename T, typename S, T ...Ns>
		using append_t = typename append<T, S, Ns...>::type;

		template <typename T, T B, T E, typename S = integral_sequence<T>>
		struct enumerate : type_is<typename enumerate<T, B + 1, E, append_t<T, S, B>>::type> {};
		template <typename T, T E, typename S>
		struct enumerate<T, E, E, S> : type_is<S> {};
		template <typename T, T B, T E>
		using enumerate_t = typename enumerate<T, B, E>::type;

		template <typename T, T N>
		using make_integral_sequence = enumerate_t<T, 0, N>;

		template <std::size_t N>
		using make_index_sequence = make_integral_sequence<std::size_t, N>;
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
		                                                    mpl::list<Ts...>>::value>
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
			                                        mpl::list<Ts...>>::value;
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

		// pre-loop stuff
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);

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
				projection2D = core::maths::Matrixf::ortho(0., dimension.width, dimension.height, 0., -1., 1.);
				projection3D = core::maths::Matrixf::perspective(core::maths::make_degree(80.), float(dimension.width) / float(dimension.height), .125, 128.);
			}
			//
			engine::graphics::poll_messages();
			// setup frame
			static core::color::hsv_t<float> tmp1{0, 1, 1};
			tmp1 += core::color::hue_t<float>{1};
			const auto tmp2 = make_rgb(tmp1);
			// glClearColor(tmp2.red(), tmp2.green(), tmp2.blue(), 0.f);
			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// setup 3D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection3D);
			glMatrixMode(GL_MODELVIEW);
			// vvvvvvvv tmp vvvvvvvv
			view3D = core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
			                              0.f, 1.f, 0.f, -1.f,
			                              0.f, 0.f, 1.f, -10.f,
			                              0.f, 0.f, 0.f, 1.f);
			// ^^^^^^^^ tmp ^^^^^^^^
			glLoadMatrix(view3D);

			glEnable(GL_DEPTH_TEST);

			// 3d
			for (auto && point : collection.get<engine::graphics::Point>())
			{
				glPointSize(point.size);

				glPushMatrix();
				glMultMatrixf(point.matrix.get());

				glColor3f(tmp2.red(), tmp2.green(), tmp2.blue());
				glBegin(GL_POINTS);
				glVertex3f(0.f, 0.f, 0.f);
				glEnd();

				glPopMatrix();
			}

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

			// setup 2D
			glMatrixMode(GL_PROJECTION);
			glLoadMatrix(projection2D);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			// glDisable(GL_DEPTH_TEST);
			glEnable(GL_DEPTH_TEST);

			// 2d
			for (auto && rectangle : collection.get<engine::graphics::Rectangle>())
			{
				glPushMatrix();
				glTranslatef(dimension.width / 2.f, dimension.height / 2.f, rectangle.z);

				glColor3f(rectangle.red, rectangle.green, rectangle.blue);
				glBegin(GL_QUADS);
				glVertex3f(rectangle.x                  , rectangle.y                   , 0.f);
				glVertex3f(rectangle.x                  , rectangle.y + rectangle.height, 0.f);
				glVertex3f(rectangle.x + rectangle.width, rectangle.y + rectangle.height, 0.f);
				glVertex3f(rectangle.x + rectangle.width, rectangle.y                   , 0.f);
				glEnd();

				glPopMatrix();
			}

			// something temporary that delays
			core::async::delay(10);

			// swap buffers
			engine::application::window::swap_buffers();
		}
		graphics_debug_trace("render_callback stopping");
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
