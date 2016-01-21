
#ifndef ENGINE_GRAPHICS_OPENGL_FONT_HPP
#define ENGINE_GRAPHICS_OPENGL_FONT_HPP

#include <config.h>

#include "../opengl.hpp"

#include <utility/string.hpp>

#include <sstream>
#include <string>

#if WINDOW_USE_USER32
# include <windows.h>
#elif WINDOW_USE_X11
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
#if WINDOW_USE_USER32
					/**
					 */
					HFONT hFont;
#elif WINDOW_USE_X11
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
				/**
				 */
				GLuint base;
#if WINDOW_USE_X11
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
				void draw(const char c) const;
				/**
				 */
				void draw(const char *text) const;
				/**
				 */
				void draw(const std::string & string) const;
				/**
				 */
				void draw(const char *const text, const unsigned int length) const;
				/**
				 */
				template <class ...Args>
				void drawt(Args && ... args) const
				{
					this->draw(utility::to_string(std::forward<Args>(args)...));
				}
			};
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_FONT_HPP */
