
#ifndef CORE_GRAPHICS_IMAGE_HPP
#define CORE_GRAPHICS_IMAGE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace core
{
	namespace graphics
	{
		enum struct ImageColor
		{
			RGB,
			RGBA
		};
		enum struct ImageFormat
		{
			PNG
		};

		class Image
		{
		private:
			int width_;
			int height_;

			int8_t bit_depth_;
			int8_t channel_count_;
			ImageColor color_;

			std::vector<char> pixels_;

		public:
			~Image();
			Image() = default;
			Image(const std::string & filename);
			Image(const std::string & filename, ImageFormat format);

		private:
			bool read_png(const char * const filename);

		public:
			int width() const { return width_; }
			int height() const { return height_; }

			int bit_depth() const { return bit_depth_; }
			int channel_count() const { return channel_count_; }
			ImageColor color() const { return color_; }

			const void * data() const { return pixels_.data(); }
		};
	}
}

#endif /* CORE_GRAPHICS_IMAGE_HPP */
