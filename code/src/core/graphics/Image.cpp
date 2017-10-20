
#include "Image.hpp"

#include <core/debug.hpp>

#include <cstdio>
#include <png.h>

namespace core
{
	namespace graphics
	{
		Image::~Image()
		{
		}
		Image::Image(const std::string & filename)
			: Image(filename, ImageFormat::PNG)
		{
		}
		Image::Image(const std::string & filename, ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::PNG:
				read_png(filename.c_str());
				break;
			default:
				debug_unreachable();
			}
		}

		bool Image::read_png(const char * const filename)
		{
			FILE * file = fopen(filename, "rb");
			if (!file)
				return false;

			png_byte sig[8];
			fread(sig, 1, 8, file);
			if (png_sig_cmp(sig, 0, 8))
			{
				fclose(file);
				return false;
			}

			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (!png_ptr)
			{
				fclose(file);
				return false;
			}

			png_infop info_ptr = png_create_info_struct(png_ptr);
			if (!info_ptr)
			{
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				fclose(file);
				return false;
			}

			png_infop end_info = png_create_info_struct(png_ptr);
			if (!end_info)
			{
				png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
				fclose(file);
				return false;
			}

			if (setjmp(png_jmpbuf(png_ptr)))
			{
				png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
				fclose(file);
				return false;
			}

			png_init_io(png_ptr, file);
			png_set_sig_bytes(png_ptr, 8);

			png_read_info(png_ptr, info_ptr);

			const int channels = png_get_channels(png_ptr, info_ptr);
			const int color_type = png_get_color_type(png_ptr, info_ptr);
			const int image_width = png_get_image_width(png_ptr, info_ptr);
			const int image_height = png_get_image_height(png_ptr, info_ptr);
			const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
			const int row_size = image_width * channels * (bit_depth / 8);
			debug_printline(core::core_channel, "texture: ", filename);
			debug_printline(core::core_channel, "channels: ", channels);
			debug_printline(core::core_channel, "color_type: ", color_type);
			debug_printline(core::core_channel, "image_width: ", image_width);
			debug_printline(core::core_channel, "image_height: ", image_height);
			debug_printline(core::core_channel, "bit_depth: ", bit_depth);
			std::vector<char> pixels(row_size * image_height);
			// rows are ordered top to bottom in PNG, but OpenGL wants it bottom to top.
			std::vector<png_bytep> rows(image_height);
			for (int i = 0; i < image_height; i++)
				rows[i] = reinterpret_cast<png_bytep>(&pixels[(image_height - 1 - i) * row_size]);

			png_read_image(png_ptr, rows.data());
			png_read_end(png_ptr, end_info);

			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
			fclose(file);

			width_ = image_width;
			height_ = image_height;

			bit_depth_ = bit_depth;
			channel_count_ = channels;
			switch (color_type)
			{
			case PNG_COLOR_TYPE_RGB:
				color_ = ImageColor::RGB;
				break;
			case PNG_COLOR_TYPE_RGBA:
				color_ = ImageColor::RGBA;
				break;
			default:
				debug_unreachable();
			}

			pixels_ = std::move(pixels);

			return true;
		}
	}
}
