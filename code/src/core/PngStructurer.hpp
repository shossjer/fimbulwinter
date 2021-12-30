#pragma once

#include "core/container/Buffer.hpp"
#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/graphics/types.hpp"
#include "core/serialization.hpp"

#include "utility/span.hpp"

#include "ful/string_modify.hpp" // ful::copy

#include <vector>

#include <png.h>

namespace core
{
	class PngStructurer
	{
	private:

		core::content & content_;

	public:

		explicit PngStructurer(core::content & content)
			: content_(content)
		{}

		template <typename T>
		void read(T & x)
		{
			if (!debug_verify(content_.size() >= 8))
				return;

			png_const_bytep ptr = static_cast<png_const_bytep>(content_.data());
			if (!debug_verify(png_sig_cmp(ptr, 0, 8) == 0, "not a png signature"))
				return;

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

			// all objects with a non-trivial destructor have to be
			// put before the setjmp
			core::container::Buffer pixels;
			std::vector<png_bytep> rows;

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4611 )
			// the microsoft compiler complains about setjmp and
			// object destruction
#endif
			if (setjmp(png_jmpbuf(png_ptr)))
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
			{
				debug_fail("png is borked");
				png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
				return;
			}

			utility::span<png_byte> view(static_cast<png_byte *>(content_.data()) + 8, content_.size() - 8);
			auto custom_read =
				[](png_structp png_ptr, png_bytep data, png_size_t size)
			{
				utility::span<png_byte> & view = *static_cast<utility::span<png_byte> *>(png_get_io_ptr(png_ptr));
				if (utility::empty(view))
				{
					png_error(png_ptr, "unexpected eol");
				}
				if (utility::size(view) < size)
				{
					png_error(png_ptr, "unexpected eol");
				}
				ful::copy(ful::first(view, size), data, data + size);
				view = utility::drop(view, size);
			};
			png_set_read_fn(png_ptr, &view, custom_read);
			png_set_sig_bytes(png_ptr, 8);

			png_read_info(png_ptr, info_ptr);

			const int channels = png_get_channels(png_ptr, info_ptr);
			const int color_type = png_get_color_type(png_ptr, info_ptr);
			const int image_width = png_get_image_width(png_ptr, info_ptr);
			const int image_height = png_get_image_height(png_ptr, info_ptr);
			const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
			const int row_size = image_width * channels * (bit_depth / 8);
			debug_printline(core::core_channel, "texture: ", content_.filepath());
			debug_printline(core::core_channel, "channels: ", channels);
			debug_printline(core::core_channel, "color_type: ", color_type);
			debug_printline(core::core_channel, "image_width: ", image_width);
			debug_printline(core::core_channel, "image_height: ", image_height);
			debug_printline(core::core_channel, "bit_depth: ", bit_depth);

			if (!debug_verify(pixels.reshape<uint8_t>(row_size * image_height)))
				return; // todo cleanup

			rows.resize(image_height);
			// rows are ordered top to bottom in PNG, but OpenGL wants it bottom to top.
			for (int i = 0; i < image_height; i++)
			{
				rows[i] = reinterpret_cast<png_bytep>(pixels.data() + (image_height - 1 - i) * row_size);
			}

			png_read_image(png_ptr, rows.data());
			png_read_end(png_ptr, end_info);

			png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

			using core::serialize;
			serialize<member_table<T>::find(ful::cstr_utf8("width"))>(x, image_width);
			serialize<member_table<T>::find(ful::cstr_utf8("height"))>(x, image_height);

			serialize<member_table<T>::find(ful::cstr_utf8("bit_depth"))>(x, [bit_depth]()
			{
				switch (bit_depth)
				{
				case 1:
				case 2:
				case 4:
				case 8:
				case 16:
					return static_cast<graphics::BitDepth>(bit_depth);
				default:
					debug_unreachable("unknown bit depth");
				}
			});

			serialize<member_table<T>::find(ful::cstr_utf8("channel_count"))>(x, [channels]()
			{
				switch (channels)
				{
				case 1:
				case 2:
				case 3:
				case 4:
					return static_cast<graphics::ChannelCount>(channels);
				default:
					debug_unreachable("unknown number of channels");
				}
			});

			serialize<member_table<T>::find(ful::cstr_utf8("color_type"))>(x, [color_type]()
			{
				switch (color_type)
				{
				case PNG_COLOR_TYPE_RGB:
					return graphics::ColorType::RGB;
				case PNG_COLOR_TYPE_RGBA:
					return graphics::ColorType::RGBA;
				default:
					debug_unreachable("unknown color type");
				}
			});

			serialize<member_table<T>::find(ful::cstr_utf8("pixel_data"))>(x, std::move(pixels));
		}
	};
}
