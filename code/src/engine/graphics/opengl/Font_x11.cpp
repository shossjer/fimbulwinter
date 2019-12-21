#include "config.h"

#if TEXT_USE_X11

#include "Font.hpp"

#include "core/debug.hpp"

#include "engine/application/window.hpp"

#include <utility>

namespace engine
{
	namespace application
	{
		extern void buildFont(window & window, XFontStruct * font_struct, unsigned int first, unsigned int last, int base);
		extern void freeFont(window & window, XFontStruct * font_struct);
		extern XFontStruct * loadFont(window & window, const char * name, int height);
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
				if (this->font_struct != nullptr)
				{
					this->free();
				}
			}
			Font::Data::Data()
				: font_struct(nullptr)
			{}
			Font::Data::Data(Data && data)
				: window_(nullptr)
				, font_struct(std::exchange(data.font_struct, nullptr))
			{}
			Font::Data & Font::Data::operator = (Data && data)
			{
				this->window_ = data.window_;
				this->font_struct = std::exchange(data.font_struct, nullptr);

				return *this;
			}

			void Font::Data::free()
			{
				debug_assert(this->font_struct != nullptr);

				freeFont(*this->window_, this->font_struct);
				this->font_struct = nullptr;
			}
			bool Font::Data::load(engine::application::window & window, const char * name, int height)
			{
				debug_assert(this->font_struct == nullptr);

				this->window_ = &window;
				this->font_struct = loadFont(window, name, height);

				return this->font_struct != nullptr;
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
				base(font.base),
				count(font.count)
			{
				font.base = -1;
			}
			Font & Font::operator = (Font && font)
			{
				this->base = font.base;
				this->count = font.count;
				font.base = -1;

				return *this;
			}

			void Font::compile(const Data & data)
			{
				debug_assert(this->base == GLuint(-1));

				const auto first = data.font_struct->min_char_or_byte2;
				const auto last = data.font_struct->max_char_or_byte2;

				this->count = last + 1;
				this->base = glGenLists(this->count);
				buildFont(*data.window_, data.font_struct, first, last, this->base);
			}
			void Font::decompile()
			{
				debug_assert(this->base != GLuint(-1));

				glDeleteLists(this->base, this->count);
				this->base = -1;
			}

			void Font::draw(int x, int y, char c) const
			{
				debug_assert(this->base != GLuint(-1));

				glRasterPos2i(x, y);
				glCallList(this->base + c);
			}
			void Font::draw(int x, int y, const char * text) const
			{
				debug_assert(this->base != GLuint(-1));

				glRasterPos2i(x, y);
				for (char c = *text; c != '\0'; c = *++text)
				{
					glCallList(this->base + c);
				}
			}
			void Font::draw(int x, int y, const std::string & string) const
			{
				debug_assert(this->base != GLuint(-1));

				glRasterPos2i(x, y);
				glListBase(this->base);
				glCallLists(string.length(), GL_UNSIGNED_BYTE, string.c_str());
			}
			void Font::draw(int x, int y, const char * text, unsigned int length) const
			{
				debug_assert(this->base != GLuint(-1));

				glRasterPos2i(x, y);
				glListBase(this->base);
				glCallLists(length, GL_UNSIGNED_BYTE, text);
			}
		}
	}
}

#endif /* TEXT_USE_X11 */
