#pragma once

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

			BitDepth bit_depth_;
			ChannelCount channel_count_;
			ColorType color_;

			core::container::Buffer pixels_;

		public:
			Image() = default;
			Image(int width, int height, BitDepth bit_depth, ChannelCount channel_count, ColorType color, core::container::Buffer && pixels)
				: width_(width)
				, height_(height)
				, bit_depth_(bit_depth)
				, channel_count_(channel_count)
				, color_(color)
				, pixels_(std::move(pixels))
			{}

		public:
			int width() const { return width_; }
			int height() const { return height_; }

			BitDepth bit_depth() const { return bit_depth_; }
			ChannelCount channel_count() const { return channel_count_; }
			ColorType color() const { return color_; }

			const void * data() const { return pixels_.data(); }
			const core::container::Buffer & pixels() const { return pixels_; }

		public:
			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("width"), &Image::width_),
					std::make_pair(utility::string_units_utf8("height"), &Image::height_),
					std::make_pair(utility::string_units_utf8("bit_depth"), &Image::bit_depth_),
					std::make_pair(utility::string_units_utf8("channel_count"), &Image::channel_count_),
					std::make_pair(utility::string_units_utf8("color_type"), &Image::color_),
					std::make_pair(utility::string_units_utf8("pixel_data"), &Image::pixels_)
					);
			}
		};
	}
}
