
#include "Font.hpp"

#if TEXT_USE_BITMAP

#include <core/debug.hpp>

namespace engine
{
	namespace application
	{
		namespace window
		{
			extern void buildFont(XFontStruct *const font_struct, const unsigned int first, const unsigned int last, const int base);
			extern void freeFont(XFontStruct *const font_struct);
			extern XFontStruct *loadFont(const char *const name, const int height);
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
				if (this->font_struct != nullptr)
				{
					this->free();
				}
			}
			Font::Data::Data() :
				font_struct(nullptr)
			{
			}
			Font::Data::Data(Data && data) :
				font_struct(data.font_struct)
			{
				data.font_struct = nullptr;
			}
			Font::Data & Font::Data::operator = (Data && data)
			{
				this->font_struct = data.font_struct;
				data.font_struct = nullptr;
				return *this;
			}

			void Font::Data::free()
			{
				debug_assert(this->font_struct != nullptr);

				engine::application::window::freeFont(this->font_struct);
				this->font_struct = nullptr;
			}
			bool Font::Data::load(const char *const name, const int height)
			{
				debug_assert(this->font_struct == nullptr);

				return (this->font_struct = engine::application::window::loadFont(name, height)) != nullptr;
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
				engine::application::window::buildFont(data.font_struct, first, last, this->base);
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
			void Font::draw(int x, int y, const char *text) const
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
