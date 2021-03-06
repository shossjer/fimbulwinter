#include "config.h"

#if GRAPHICS_USE_OPENGL && TEXT_USE_USER32

#include "Font.hpp"

#include "core/debug.hpp"

#include <utility>

namespace engine
{
	namespace application
	{
		extern void buildFont(window & window, HFONT hFont, DWORD count, DWORD listBase);
		extern void freeFont(window & window, HFONT hFont);
		extern HFONT loadFont(window & window, utility::string_view_utf8 name, int height);
	}
}

namespace
{
	constexpr auto invalid_base = GLuint(-1);
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

			Font::Data::Data()
				: hFont(nullptr)
			{}

			Font::Data::Data(Data && data)
				: window_(data.window_)
				, hFont(std::exchange(data.hFont, nullptr))
			{}

			Font::Data & Font::Data::operator = (Data && data)
			{
				this->window_ = data.window_;
				this->hFont = std::exchange(data.hFont, nullptr);

				return *this;
			}

			void Font::Data::free()
			{
				debug_assert(this->hFont != nullptr);

				freeFont(*this->window_, this->hFont);
				this->hFont = nullptr;
			}

			bool Font::Data::load(engine::application::window & window, utility::string_view_utf8 name, int height)
			{
				debug_assert(this->hFont == nullptr);

				this->window_ = &window;
				this->hFont = loadFont(window, name, height);

				return this->hFont != nullptr;
			}

			Font::~Font()
			{
				if (base != invalid_base)
				{
					this->decompile();
				}
			}

			Font::Font() :
				base(invalid_base)
			{}

			Font::Font(Font && font) :
				base(std::exchange(font.base, invalid_base))
			{}

			Font & Font::operator = (Font && font)
			{
				this->base = std::exchange(font.base, invalid_base);

				return *this;
			}

			void Font::compile(const Data & data)
			{
				debug_assert(this->base == invalid_base);

				this->base = glGenLists(256);
				buildFont(*data.window_, data.hFont, 256, this->base);
			}

			void Font::decompile()
			{
				debug_assert(this->base != invalid_base);

				glDeleteLists(this->base, 256);
				this->base = invalid_base;
			}

			void Font::draw(int x, int y, char c) const
			{
				debug_assert(this->base != invalid_base);

				glRasterPos2i(x, y);
				glCallList(this->base + c);
			}

			void Font::draw(int x, int y, const char * text) const
			{
				debug_assert(this->base != invalid_base);

				glRasterPos2i(x, y);
				for (char c = *text; c != '\0'; c = *++text)
				{
					glCallList(this->base + c);
				}
			}

			void Font::draw(int x, int y, const std::string & string) const
			{
				debug_assert(this->base != invalid_base);

				glRasterPos2i(x, y);
				glListBase(this->base);
				glCallLists(string.length(), GL_UNSIGNED_BYTE, string.c_str());
			}

			void Font::draw(int x, int y, const char * text, std::ptrdiff_t length) const
			{
				debug_assert(this->base != invalid_base);

				glRasterPos2i(x, y);
				glListBase(this->base);
				glCallLists(length, GL_UNSIGNED_BYTE, text);
			}
		}
	}
}

#endif /* TEXT_USE_USER32 */
