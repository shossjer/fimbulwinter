
#ifndef CORE_GRAPHICS_IMAGE_HPP
#define CORE_GRAPHICS_IMAGE_HPP

#include "core/container/Buffer.hpp"
#include "core/graphics/types.hpp"
#include "core/serialization.hpp"

#include <cstdint>
#include <vector>

namespace core
{
	namespace graphics
	{
		class Image
		{
		private:
			int width_;
			int height_;

			int8_t bit_depth_;
			int8_t channel_count_;
			ColorType color_;

			core::container::Buffer pixels_;

		public:
			int width() const { return width_; }
			int height() const { return height_; }

			int bit_depth() const { return bit_depth_; }
			int channel_count() const { return channel_count_; }
			ColorType color() const { return color_; }

			const void * data() const { return pixels_.data(); }
			const core::container::Buffer & pixels() const { return pixels_; }

		public:
			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("width"), &Image::width_),
					std::make_pair(utility::string_view("height"), &Image::height_),
					std::make_pair(utility::string_view("bit_depth"), &Image::bit_depth_),
					std::make_pair(utility::string_view("channel_count"), &Image::channel_count_),
					std::make_pair(utility::string_view("color_type"), &Image::color_),
					std::make_pair(utility::string_view("pixel_data"), &Image::pixels_)
					);
			}
		};
	}
}

#endif /* CORE_GRAPHICS_IMAGE_HPP */
