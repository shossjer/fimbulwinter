
#ifndef ENGINE_GRAPHICS_OPENGL_COLOR_HPP
#define ENGINE_GRAPHICS_OPENGL_COLOR_HPP

#include <core/debug.hpp>
#include <engine/graphics/opengl.hpp>

#include <array>
#include <type_traits>
#include <utility>

namespace engine
{
	namespace graphics
	{
		namespace opengl
		{
			namespace detail
			{
				inline void glColor(const std::array<GLbyte, 3> &channels)
				{
					glColor3bv(channels.data());
				}
				inline void glColor(const std::array<GLshort, 3> &channels)
				{
					glColor3sv(channels.data());
				}
				inline void glColor(const std::array<GLint, 3> &channels)
				{
					glColor3iv(channels.data());
				}
				inline void glColor(const std::array<GLfloat, 3> &channels)
				{
					glColor3fv(channels.data());
				}
				inline void glColor(const std::array<GLdouble, 3> &channels)
				{
					glColor3dv(channels.data());
				}
				inline void glColor(const std::array<GLubyte, 3> &channels)
				{
					glColor3ubv(channels.data());
				}
				inline void glColor(const std::array<GLushort, 3> &channels)
				{
					glColor3usv(channels.data());
				}
				inline void glColor(const std::array<GLuint, 3> &channels)
				{
					glColor3uiv(channels.data());
				}
				inline void glColor(const std::array<GLbyte, 4> &channels)
				{
					glColor4bv(channels.data());
				}
				inline void glColor(const std::array<GLshort, 4> &channels)
				{
					glColor4sv(channels.data());
				}
				inline void glColor(const std::array<GLint, 4> &channels)
				{
					glColor4iv(channels.data());
				}
				inline void glColor(const std::array<GLfloat, 4> &channels)
				{
					glColor4fv(channels.data());
				}
				inline void glColor(const std::array<GLdouble, 4> &channels)
				{
					glColor4dv(channels.data());
				}
				inline void glColor(const std::array<GLubyte, 4> &channels)
				{
					glColor4ubv(channels.data());
				}
				inline void glColor(const std::array<GLushort, 4> &channels)
				{
					glColor4usv(channels.data());
				}
				inline void glColor(const std::array<GLuint, 4> &channels)
				{
					glColor4uiv(channels.data());
				}

				inline GLbyte invert_channel(const GLbyte channel)
				{
					debug_assert(channel >= 0);

					return 0xff - channel;
				}
				inline GLshort invert_channel(const GLshort channel)
				{
					debug_assert(channel >= 0);

					return 0xffff - channel;
				}
				inline GLint invert_channel(const GLint channel)
				{
					debug_assert(channel >= 0);

					return 0xffffffff - channel;
				}
				inline float invert_channel(const float channel)
				{
					debug_assert(channel >= 0.f);
					debug_assert(channel <= 1.f);

					return 1.f - channel;
				}
				inline double invert_channel(const double channel)
				{
					debug_assert(channel >= 0.);
					debug_assert(channel <= 1.);

					return 1. - channel;
				}
				inline GLubyte invert_channel(const GLubyte channel)
				{
					return 0xff - channel;
				}
				inline GLushort invert_channel(const GLushort channel)
				{
					return 0xffff - channel;
				}
				inline GLuint invert_channel(const GLuint channel)
				{
					return 0xffffffff - channel;
				}
			}
			/**
			 */
			template <std::size_t D, class T>
			class Color
			{
			private:
				/**
				 */
				std::array<T, D> channels;

			public:
				/**
				 */
				Color() = default;
				/**
				 */
				template <class ...Ps,
				          class = typename std::enable_if<sizeof...(Ps) == D>::type>
				Color(Ps &&...ps) :
					channels{{T(std::forward<Ps>(ps))...}}
				{
				}

			public:
				/**
				 */
				T &operator [] (const std::size_t i)
				{
					debug_assert(i < D);

					return this->channels[i];
				}
				/**
				 */
				const T &operator [] (const std::size_t i) const
				{
					debug_assert(i < D);

					return this->channels[i];
				}

			public:
				/**
				 */
				Color<D, T> invert() const
				{
					Color<D, T> color;
					{
						for (std::size_t i = 0; i < D; ++i)
						{
							color.channels[i] = detail::invert_channel(this->channels[i]);
						}
					}
					return color;
				}

			public:
				/**
				 */
				friend void glColor(const Color<D, T> &color)
				{
					detail::glColor(color.channels);
				}
			};
			/**
			 */
			template <class T>
			Color<3, T> make_color(const T r, const T g, const T b)
			{
				return Color<3, T>(r, g, b);
			}
			/**
			 */
			template <class T>
			Color<4, T> make_color(const T r, const T g, const T b, const T a)
			{
				return Color<4, T>(r, g, b, a);
			}

			using Color3b = Color<3, GLbyte>;
			using Color3s = Color<3, GLshort>;
			using Color3i = Color<3, GLint>;
			using Color3f = Color<3, GLfloat>;
			using Color3d = Color<3, GLdouble>;
			using Color3ub = Color<3, GLubyte>;
			using Color3us = Color<3, GLushort>;
			using Color3ui = Color<3, GLuint>;
			using Color4b = Color<4, GLbyte>;
			using Color4s = Color<4, GLshort>;
			using Color4i = Color<4, GLint>;
			using Color4f = Color<4, GLfloat>;
			using Color4d = Color<4, GLdouble>;
			using Color4ub = Color<4, GLubyte>;
			using Color4us = Color<4, GLushort>;
			using Color4ui = Color<4, GLuint>;
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_COLOR_HPP */
