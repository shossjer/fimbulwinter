
#ifndef CORE_GRAPHICS_TYPES_HPP
#define CORE_GRAPHICS_TYPES_HPP

namespace core
{
	namespace graphics
	{
		enum struct BitDepth
		{
			one = 1,
			two = 2,
			four = 4,
			eight = 8,
			sixteen = 16,
		};

		enum struct ChannelCount
		{
			one = 1,
			two = 2,
			three = 3,
			four = 4,
		};

		enum struct ColorType
		{
			R,
			RGB,
			RGBA,
		};
	}
}

#endif /* CORE_GRAPHICS_TYPES_HPP */
