
#include "Font.hpp"

#if TEXT_USE_USER32

#include <core/debug.hpp>

namespace engine
{
	namespace application
	{
		namespace window
		{
			extern void buildFont(HFONT hFont, DWORD count, DWORD listBase);
			extern void freeFont(HFONT hFont);
			extern HFONT loadFont(const char *const name, const int height);
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
				if (this->hFont != nullptr)
				{
					this->free();
				}
			}
			Font::Data::Data() :
				hFont(nullptr)
			{
			}
			Font::Data::Data(Data && data) :
				hFont(data.hFont)
			{
				data.hFont = nullptr;
			}
			Font::Data & Font::Data::operator = (Data && data)
			{
				this->hFont = data.hFont;
				data.hFont = nullptr;
				return *this;
			}

			void Font::Data::free()
			{
				debug_assert(this->hFont != nullptr);

				engine::application::window::freeFont(this->hFont);
				this->hFont = nullptr;
			}
			bool Font::Data::load(const char *const name, const int height)
			{
				debug_assert(this->hFont == nullptr);

				return (this->hFont = engine::application::window::loadFont(name, height)) != nullptr;
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
				base(font.base)
			{
				font.base = -1;
			}
			Font & Font::operator = (Font && font)
			{
				this->base = font.base;
				font.base = -1;

				return *this;
			}

			void Font::compile(const Data & data)
			{
				debug_assert(this->base == GLuint(-1));

				this->base = glGenLists(256);
				engine::application::window::buildFont(data.hFont, 256, this->base);
			}
			void Font::decompile()
			{
				debug_assert(this->base != GLuint(-1));

				glDeleteLists(this->base, 256);
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

#endif /* TEXT_USE_USER32 */
