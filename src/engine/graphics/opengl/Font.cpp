
#include "Font.hpp"

#include <core/debug.hpp>

namespace engine
{
	namespace application
	{
		namespace window
		{
#if WINDOW_USE_USER32
			extern void buildFont(HFONT hFont, DWORD count, DWORD listBase);
			extern void freeFont(HFONT hFont);
			extern HFONT loadFont(const char *const name, const int height);
#elif WINDOW_USE_X11
			extern void buildFont(XFontStruct *const font_struct, const unsigned int first, const unsigned int last, const int base);
			extern void freeFont(XFontStruct *const font_struct);
			extern XFontStruct *loadFont(const char *const name, const int height);
#endif
		}
	}
}

namespace engine
{
	namespace graphics
	{
		namespace opengl
		{
			Font::Data::~Data()
			{
#if WINDOW_USE_USER32
				if (this->hFont != nullptr)
#elif WINDOW_USE_X11
				if (this->font_struct != nullptr)
#endif
				{
					this->free();
				}
			}
			Font::Data::Data() :
#if WINDOW_USE_USER32
				hFont(nullptr)
#elif WINDOW_USE_X11
				font_struct(nullptr)
#endif
			{
			}
			Font::Data::Data(Data && data) :
#if WINDOW_USE_USER32
				hFont(data.hFont)
#elif WINDOW_USE_X11
				font_struct(data.font_struct)
#endif
			{
#if WINDOW_USE_USER32
				data.hFont = nullptr;
#elif WINDOW_USE_X11
				data.font_struct = nullptr;
#endif
			}
			Font::Data & Font::Data::operator = (Data && data)
			{
#if WINDOW_USE_USER32
				this->hFont = data.hFont;
				data.hFont = nullptr;
#elif WINDOW_USE_X11
				this->font_struct = data.font_struct;
				data.font_struct = nullptr;
#endif
				return *this;
			}

			void Font::Data::free()
			{
#if WINDOW_USE_USER32
				debug_assert(this->hFont != nullptr);

				engine::application::window::freeFont(this->hFont);
				this->hFont = nullptr;
#elif WINDOW_USE_X11
				debug_assert(this->font_struct != nullptr);

				engine::application::window::freeFont(this->font_struct);
				this->font_struct = nullptr;
#endif
			}
			bool Font::Data::load(const char *const name, const int height)
			{
#if WINDOW_USE_USER32
				debug_assert(this->hFont == nullptr);

				return (this->hFont = engine::application::window::loadFont(name, height)) != nullptr;
#elif WINDOW_USE_X11
				debug_assert(this->font_struct == nullptr);

				return (this->font_struct = engine::application::window::loadFont(name, height)) != nullptr;
#endif
			}

			Font::~Font()
			{
				if (base != GLuint(-1))
				{
					this->decompile();
				}
			}
			Font::Font() :
				base(-1)
			{
			}
			Font::Font(Font && font) :
#if WINDOW_USE_USER32
				base(font.base)
#elif WINDOW_USE_X11
				base(font.base),
				count(font.count)
#endif
			{
				font.base = -1;
			}
			Font & Font::operator = (Font && font)
			{
				this->base = font.base;
#if WINDOW_USE_X11
				this->count = font.count;
#endif
				font.base = -1;

				return *this;
			}

			void Font::compile(const Data & data)
			{
				debug_assert(this->base == GLuint(-1));

#if WINDOW_USE_USER32
				this->base = glGenLists(256);
				engine::application::window::buildFont(data.hFont, 256, this->base);
#elif WINDOW_USE_X11
				const auto first = data.font_struct->min_char_or_byte2;
				const auto last = data.font_struct->max_char_or_byte2;

				this->count = last + 1;
				this->base = glGenLists(this->count);
				engine::application::window::buildFont(data.font_struct, first, last, this->base);
#endif
			}
			void Font::decompile()
			{
				debug_assert(this->base != GLuint(-1));

#if WINDOW_USE_USER32
				glDeleteLists(this->base, 256);
#elif WINDOW_USE_X11
				glDeleteLists(this->base, this->count);
#endif
				this->base = -1;
			}

			void Font::draw(const char c) const
			{
				debug_assert(this->base != GLuint(-1));

				glCallList(this->base + c);
			}
			void Font::draw(const char *text) const
			{
				debug_assert(this->base != GLuint(-1));

				for (char c = *text; c != '\0'; c = *++text)
				{
					glCallList(this->base + c);
				}
			}
			void Font::draw(const std::string & string) const
			{
				debug_assert(this->base != GLuint(-1));

				glListBase(this->base);
				glCallLists(string.length(), GL_UNSIGNED_BYTE, string.c_str());
			}
			void Font::draw(const char *const text, const unsigned int length) const
			{
				debug_assert(this->base != GLuint(-1));

				glListBase(this->base);
				glCallLists(length, GL_UNSIGNED_BYTE, text);
			}
		}
	}
}
