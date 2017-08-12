
#ifndef ENGINE_GRAPHICS_OPENGL_FONT_HPP
#define ENGINE_GRAPHICS_OPENGL_FONT_HPP

#include <config.h>

#include "../opengl.hpp"

#include <utility/string.hpp>

# include <string>

#if TEXT_USE_FREETYPE
# include <ft2build.h>
# include FT_FREETYPE_H
#elif TEXT_USE_USER32
# include <windows.h>
#elif TEXT_USE_X11
# include <X11/Xlib.h>
#endif

namespace engine
{
	namespace graphics
	{
		namespace opengl
		{
			/**
			 */
			class Font
			{
			public:
				/**
				 */
				class Data
				{
				private:
#if TEXT_USE_FREETYPE
					/**
					 */
					struct Param
					{
						int16_t bitmap_left;
						int16_t bitmap_top;
						int16_t advance_x;
						int16_t advance_y;
					};
					/**
					 */
					FT_Library library;
					/**
					 */
					FT_Face face;
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
					std::vector<unsigned char> pixels;
					/**
					 */
					std::vector<Param> params;
#elif TEXT_USE_USER32
					/**
					 */
					HFONT hFont;
#elif TEXT_USE_X11
					/**
					 */
					XFontStruct *font_struct;
#endif

				public:
					/**  */
					~Data();
					/**  */
					Data();
					/**  */
					Data(const Data & data) = delete;
					/**  */
					Data & operator = (const Data & data) = delete;
					/**  */
					Data(Data && data);
					/**  */
					Data & operator = (Data && data);

				public:
					/**
					 */
					void free();
					/**
					 */
					bool load(const char *const name, const int height);

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
				std::vector<Data::Param> params;
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
				void draw(int x, int y, const char *text) const;
				/**
				 */
				void draw(int x, int y, const std::string & string) const;
				/**
				 */
				void draw(int x, int y, const char * text, unsigned int length) const;
				/**
				 */
				template <class ...Args>
				void drawt(int x, int y, Args && ... args) const
				{
					draw(x, y, utility::to_string(std::forward<Args>(args)...));
				}
			};
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_FONT_HPP */
