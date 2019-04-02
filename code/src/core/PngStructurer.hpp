
#ifndef CORE_PNGSTRUCTURER_HPP
#define CORE_PNGSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/graphics/types.hpp"
#include "core/ReadStream.hpp"
#include "core/serialization.hpp"

#include <vector>

#include <png.h>

namespace core
{
	class PngStructurer
	{
	private:
		ReadStream read_stream_;

	public:
		PngStructurer(ReadStream && read_stream)
			: read_stream_(std::move(read_stream))
		{}

		template <typename T>
		void read(T & x)
		{
			png_byte sig[8];
			const auto sig_read = read_stream_.read_block(reinterpret_cast<char *>(sig), 8);
			debug_assert(sig_read == 8);
			if (png_sig_cmp(sig, 0, 8))
			{
				debug_fail("not a png signature");
				return;
			}

			png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (!png_ptr)
			{
				debug_fail("cannot create png read struct");
				return;
			}

			png_infop info_ptr = png_create_info_struct(png_ptr);
			if (!info_ptr)
			{
				debug_fail("cannot create png info struct");
				png_destroy_read_struct(&png_ptr, NULL, NULL);
				return;
			}

			png_infop end_info = png_create_info_struct(png_ptr);
			if (!end_info)
			{
				debug_fail("cannot create png info struct");
				png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
				return;
			}

			if (setjmp(png_jmpbuf(png_ptr)))
			{
				debug_fail("png is borked");
				png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
				return;
			}

			auto custom_read =
				[](png_structp png_ptr, png_bytep data, png_size_t size)
				{
					ReadStream & read_stream = *static_cast<ReadStream *>(png_get_io_ptr(png_ptr));
					if (!read_stream.valid())
					{
						png_error(png_ptr, "unexpected eol");
					}
					const auto amount_read = read_stream.read_block(reinterpret_cast<char * >(data), size);
					if (amount_read < size)
					{
						png_error(png_ptr, "unexpected eol");
					}
				};
			png_set_read_fn(png_ptr, &read_stream_, custom_read);
			png_set_sig_bytes(png_ptr, 8);

			png_read_info(png_ptr, info_ptr);

			const int channels = png_get_channels(png_ptr, info_ptr);
			const int color_type = png_get_color_type(png_ptr, info_ptr);
			const int image_width = png_get_image_width(png_ptr, info_ptr);
			const int image_height = png_get_image_height(png_ptr, info_ptr);
			const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
			const int row_size = image_width * channels * (bit_depth / 8);
			debug_printline(core::core_channel, "texture: ", read_stream_.filename);
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

			if (member_table<T>::has("width"))
			{
				member_table<T>::call("width", x, TryAssign<int>(image_width));
			}
			if (member_table<T>::has("height"))
			{
				member_table<T>::call("height", x, TryAssign<int>(image_height));
			}

			if (member_table<T>::has("bit_depth"))
			{
				member_table<T>::call("bit_depth", x, TryAssign<int>(bit_depth));
			}
			if (member_table<T>::has("channel_count"))
			{
				member_table<T>::call("channel_count", x, TryAssign<int>(channels));
			}
			if (member_table<T>::has("color_type"))
			{
				member_table<T>::call("color_type", x, [&](auto & y){ read_color_type(color_type, y); });
			}

			if (member_table<T>::has("pixel_data"))
			{
				member_table<T>::call("pixel_data", x, TryAssign<std::vector<char> &&>(std::move(pixels)));
			}
		}
	private:
		void read_color_type(int color_type, graphics::ColorType & x)
		{
			switch (color_type)
			{
			case PNG_COLOR_TYPE_RGB:
				x = graphics::ColorType::RGB;
				break;
			case PNG_COLOR_TYPE_RGBA:
				x = graphics::ColorType::RGBA;
				break;
			default:
				debug_fail("unknown color type");
			}
		}
		template <typename T>
		void read_color_type(int, T &)
		{
			debug_fail("unknown type for color type");
		}
	};
}

#endif /* CORE_PNGSTRUCTURER_HPP */
