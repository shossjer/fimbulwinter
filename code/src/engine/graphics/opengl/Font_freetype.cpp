#include "config.h"

#if TEXT_USE_FREETYPE

#include "Font.hpp"

#include "core/debug.hpp"

#include "utility/bitmanip.hpp"
#include "utility/ranges.hpp"

#include <cmath>
#include <numeric>

namespace
{
	int smallest_power_of_two(int minimum)
	{
		return utility::clp2(minimum);
	}

	constexpr auto invalid_id = GLuint(-1);
}

namespace engine
{
	namespace graphics
	{
		namespace opengl
		{
			Font::Data::~Data()
			{
				if (library)
				{
					if (face)
					{
						free();
					}
					FT_Done_FreeType(library);
				}
			}

			Font::Data::Data()
				: face(nullptr)
			{
				if (FT_Init_FreeType(&library))
					debug_fail();
			}

			Font::Data::Data(Data && data)
				: library(data.library)
				, face(data.face)
			{
				data.library = nullptr;
				data.face = nullptr;
			}

			Font::Data & Font::Data::operator = (Data && data)
			{
				library = data.library;
				face = data.face;

				data.library = nullptr;
				data.face = nullptr;

				return *this;
			}

			void Font::Data::free()
			{
				debug_assert(face != nullptr);

				FT_Done_Face(face);
				face = nullptr;
			}

			bool Font::Data::load(const char * name, int height)
			{
				debug_assert(face == nullptr);

				if (FT_New_Face(library, name, 0, &face))
				{
					debug_fail();
					return false;
				}

				if (FT_Select_Charmap(face, FT_ENCODING_UNICODE))
				{
					debug_fail();
					free();
					return false;
				}
				if (FT_Set_Pixel_Sizes(face, 0, height))
				{
					debug_fail();
					free();
					return false;
				}

				max_bitmap_width = 0;
				max_bitmap_height = 0;
				for (int i = 0; i < 256; i++)
				{
					if (FT_Load_Char(face, i, FT_LOAD_RENDER))
					{
						debug_fail();
						free();
						return false;
					}

					max_bitmap_width = std::max(debug_cast<int16_t>(face->glyph->bitmap.width), max_bitmap_width);
					max_bitmap_height = std::max(debug_cast<int16_t>(face->glyph->bitmap.rows), max_bitmap_height);
				}

				const int estimated_area = max_bitmap_width * max_bitmap_height * 256;
				texture_size = smallest_power_of_two(static_cast<int>(std::sqrt(estimated_area)));
				do
				{
					const int max_number_of_bitmaps_in_x = texture_size / max_bitmap_width;
					const int max_number_of_bitmaps_in_y = texture_size / max_bitmap_height;
					const int max_number_of_bitmaps = max_number_of_bitmaps_in_x * max_number_of_bitmaps_in_y;
					if (max_number_of_bitmaps >= 256)
						break;
					texture_size *= 2;
				}
				while (true);

				const int max_number_of_bitmaps_in_x = texture_size / max_bitmap_width;
				pixels.resize(texture_size * texture_size, 0);
				params.resize(256);
				for (int i = 0; i < 256; i++)
				{
					if (FT_Load_Char(face, i, FT_LOAD_RENDER))
					{
						debug_fail();
						free();
						return false;
					}

					const int bitmap_slot_x = i % max_number_of_bitmaps_in_x;
					const int bitmap_slot_y = i / max_number_of_bitmaps_in_x;
					const int pixel_offset = bitmap_slot_x * max_bitmap_width + bitmap_slot_y * max_bitmap_height * texture_size;
					for (std::ptrdiff_t y : ranges::index_sequence(face->glyph->bitmap.rows))
					{
						for (std::ptrdiff_t x : ranges::index_sequence(face->glyph->bitmap.width))
						{
							pixels[pixel_offset + x + y * texture_size] = face->glyph->bitmap.buffer[x + y * face->glyph->bitmap.width];
						}
					}

					params[i].bitmap_left = debug_cast<int16_t>(face->glyph->bitmap_left);
					params[i].bitmap_top = debug_cast<int16_t>(face->glyph->bitmap_top);
					params[i].advance_x = debug_cast<int16_t>(face->glyph->advance.x >> 6);
					params[i].advance_y = debug_cast<int16_t>(face->glyph->advance.y >> 6);
				}
				return true;
			}

			Font::~Font()
			{
				if (id != invalid_id)
				{
					decompile();
				}
			}

			Font::Font()
				: id(invalid_id)
			{}

			Font::Font(Font && font)
				: id(std::exchange(font.id, invalid_id))
				, max_bitmap_width(font.max_bitmap_width)
				, max_bitmap_height(font.max_bitmap_height)
				, texture_size(font.texture_size)
				, params(std::move(font.params))
			{}

			Font & Font::operator = (Font && font)
			{
				id = std::exchange(font.id, invalid_id);
				max_bitmap_width = font.max_bitmap_width;
				max_bitmap_height = font.max_bitmap_height;
				texture_size = font.texture_size;
				params = std::move(font.params);

				return *this;
			}

			void Font::compile(const Data & data)
			{
				debug_assert(id == invalid_id);

				glEnable(GL_TEXTURE_2D);

				glGenTextures(1, &id);
				glBindTexture(GL_TEXTURE_2D, id);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, data.texture_size, data.texture_size, 0, GL_RED, GL_UNSIGNED_BYTE, data.pixels.data());

				glDisable(GL_TEXTURE_2D);

				max_bitmap_width = data.max_bitmap_width;
				max_bitmap_height = data.max_bitmap_height;
				texture_size = data.texture_size;
				params = data.params;
			}

			void Font::decompile()
			{
				debug_assert(id != invalid_id);

				glDeleteTextures(1, &id);
				id = invalid_id;
			}

			void Font::draw(int x, int y, char c) const
			{
				debug_assert(id != invalid_id);

				glEnable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);

				glDepthMask(GL_FALSE);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindTexture(GL_TEXTURE_2D, id);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				const int max_number_of_bitmaps_in_x = texture_size / max_bitmap_width;
				glBegin(GL_QUADS);
				const int bitmap_slot_x = c % max_number_of_bitmaps_in_x;
				const int bitmap_slot_y = c / max_number_of_bitmaps_in_x;
				const int ox = x - params[c].bitmap_left;
				const int oy = y - params[c].bitmap_top;
				glTexCoord2f(float(bitmap_slot_x) / float(texture_size), float(bitmap_slot_y) / float(texture_size));
				glVertex2i(ox, oy);
				glTexCoord2f(float(bitmap_slot_x) / float(texture_size), float(bitmap_slot_y + 1) / float(texture_size));
				glVertex2i(ox, oy + max_bitmap_height);
				glTexCoord2f(float(bitmap_slot_x + 1) / float(texture_size), float(bitmap_slot_y + 1) / float(texture_size));
				glVertex2i(ox + max_bitmap_width, oy + max_bitmap_height);
				glTexCoord2f(float(bitmap_slot_x + 1) / float(texture_size), float(bitmap_slot_y) / float(texture_size));
				glVertex2i(ox + max_bitmap_width, oy);
				glEnd();

				glDepthMask(GL_TRUE);

				glDisable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}

			void Font::draw(int x, int y, const char * text) const
			{
				debug_assert(id != invalid_id);

				glEnable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);

				glDepthMask(GL_FALSE);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindTexture(GL_TEXTURE_2D, id);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				const int max_number_of_bitmaps_in_x = texture_size / max_bitmap_width;
				glBegin(GL_QUADS);
				for (char c = *text; c; c = *++text)
				{
					const int bitmap_slot_x = c % max_number_of_bitmaps_in_x;
					const int bitmap_slot_y = c / max_number_of_bitmaps_in_x;
					const int ox = x - params[c].bitmap_left;
					const int oy = y - params[c].bitmap_top;
					glTexCoord2f(float(bitmap_slot_x * max_bitmap_width) / float(texture_size), float(bitmap_slot_y * max_bitmap_height) / float(texture_size));
					glVertex2i(ox, oy);
					glTexCoord2f(float(bitmap_slot_x * max_bitmap_width) / float(texture_size), float((bitmap_slot_y + 1) * max_bitmap_height) / float(texture_size));
					glVertex2i(ox, oy + max_bitmap_height);
					glTexCoord2f(float((bitmap_slot_x + 1) * max_bitmap_width) / float(texture_size), float((bitmap_slot_y + 1) * max_bitmap_height) / float(texture_size));
					glVertex2i(ox + max_bitmap_width, oy + max_bitmap_height);
					glTexCoord2f(float((bitmap_slot_x + 1) * max_bitmap_width) / float(texture_size), float(bitmap_slot_y * max_bitmap_height) / float(texture_size));
					glVertex2i(ox + max_bitmap_width, oy);
					x += params[c].advance_x;
					y += params[c].advance_y;
				}
				glEnd();

				glDepthMask(GL_TRUE);

				glDisable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}

			void Font::draw(int x, int y, const std::string & string) const
			{
				draw(x, y, string.c_str(), string.length());
			}

			void Font::draw(int x, int y, const char * text, std::ptrdiff_t length) const
			{
				debug_assert(id != invalid_id);

				glEnable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);

				glDepthMask(GL_FALSE);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindTexture(GL_TEXTURE_2D, id);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				const int max_number_of_bitmaps_in_x = texture_size / max_bitmap_width;
				glBegin(GL_QUADS);
				for (int i = 0; i < length; i++)
				{
					const char c = text[i];
					const int bitmap_slot_x = c % max_number_of_bitmaps_in_x;
					const int bitmap_slot_y = c / max_number_of_bitmaps_in_x;
					const int ox = x - params[c].bitmap_left;
					const int oy = y - params[c].bitmap_top;
					glTexCoord2f(float(bitmap_slot_x * max_bitmap_width) / float(texture_size), float(bitmap_slot_y * max_bitmap_height) / float(texture_size));
					glVertex2i(ox, oy);
					glTexCoord2f(float(bitmap_slot_x * max_bitmap_width) / float(texture_size), float((bitmap_slot_y + 1) * max_bitmap_height) / float(texture_size));
					glVertex2i(ox, oy + max_bitmap_height);
					glTexCoord2f(float((bitmap_slot_x + 1) * max_bitmap_width) / float(texture_size), float((bitmap_slot_y + 1) * max_bitmap_height) / float(texture_size));
					glVertex2i(ox + max_bitmap_width, oy + max_bitmap_height);
					glTexCoord2f(float((bitmap_slot_x + 1) * max_bitmap_width) / float(texture_size), float(bitmap_slot_y * max_bitmap_height) / float(texture_size));
					glVertex2i(ox + max_bitmap_width, oy);
					x += params[c].advance_x;
					y += params[c].advance_y;
				}
				glEnd();

				glDepthMask(GL_TRUE);

				glDisable(GL_TEXTURE_2D);
				glDisable(GL_BLEND);
			}
		}
	}
}

#endif /* TEXT_USE_FREETYPE */
