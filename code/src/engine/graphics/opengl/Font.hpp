#pragma once

#include "config.h"

#include "engine/graphics/opengl.hpp"

#include "utility/container/array.hpp"

#include "ful/cstr.hpp"

#if TEXT_USE_FREETYPE
# include <ft2build.h>
# include FT_FREETYPE_H
#elif TEXT_USE_USER32
# include <windows.h>
#elif TEXT_USE_X11
# include <X11/Xlib.h>
#endif

#if TEXT_USE_USER32 || TEXT_USE_X11
namespace engine
{
	namespace application
	{
		class window;
	}
}
#endif

namespace engine
{
	namespace graphics
	{
		namespace opengl
		{
			class Font
			{
			public:
				class Data
				{
				private:
#if TEXT_USE_FREETYPE
					struct Param
					{
						int16_t bitmap_left;
						int16_t bitmap_top;
						int16_t advance_x;
						int16_t advance_y;
					};

					FT_Library library;
					FT_Face face;
					int16_t max_bitmap_width;
					int16_t max_bitmap_height;
					int32_t texture_size;
					utility::heap_array<unsigned char> pixels;
					utility::heap_array<Param> params;
#elif TEXT_USE_USER32
					engine::application::window * window_;
					HFONT hFont;
#elif TEXT_USE_X11
					engine::application::window * window_;
					XFontStruct *font_struct;
#endif

				public:
					~Data();
					Data();
					Data(Data && data);
					Data & operator = (Data && data);

				public:
					void free();
#if TEXT_USE_FREETYPE
					bool load(ful::cstr_utf8 name, int height);
#elif TEXT_USE_USER32 || TEXT_USE_X11
					bool load(engine::application::window & window, ful::cstr_utf8 name, int height);
#endif

				public:
					friend class Font;

				};

			private:
#if TEXT_USE_FREETYPE
				/**
				 */
				GLuint id;
				/**
				 */
				int16_t max_bitmap_width;
				/**
				 */
				int16_t max_bitmap_height;
				/**
				 */
				int32_t texture_size;
				/**
				 */
				utility::heap_array<Data::Param> params;
#elif TEXT_USE_USER32
				/**
				 */
				GLuint base;
#elif TEXT_USE_X11
				/**
				 */
				GLuint base;
				/**
				 */
				unsigned int count;
#endif

			public:
				/**  */
				~Font();
				/**  */
				Font();
				/**  */
				Font(const Font & font) = delete;
				/**  */
				Font & operator = (const Font & font) = delete;
				/**  */
				Font(Font && font);
				/**  */
				Font & operator = (Font && font);

			public:
				/**
				 */
				void compile(const Data & data);
				/**
				 */
				void decompile();

				/**
				 */
				void draw(int x, int y, char c) const;
				/**
				 */
				void draw(int x, int y, const char * text) const;
				/**
				 */
				void draw(int x, int y, ful::view_utf8 str) const;
				/**
				 */
				void draw(int x, int y, const char * text, std::ptrdiff_t length) const;

			};
		}
	}
}
