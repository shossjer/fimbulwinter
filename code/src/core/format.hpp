
#ifndef CORE_FORMAT_HPP
#define CORE_FORMAT_HPP

#include <utility/type_traits.hpp>

#include <cstdint>

namespace core
{
	enum class Format : uint32_t
	{
		// this should be done differently, but it is ok for now
		UINT16 = 0x1403,
		UINT32 = 0x1405,
		SINGLE = 0x1406
	};
// /* Data types */
// #define GL_BYTE					0x1400
// #define GL_UNSIGNED_BYTE			0x1401
// #define GL_SHORT				0x1402
// #define GL_UNSIGNED_SHORT			0x1403
// #define GL_INT					0x1404
// #define GL_UNSIGNED_INT				0x1405
// #define GL_FLOAT				0x1406
// #define GL_2_BYTES				0x1407
// #define GL_3_BYTES				0x1408
// #define GL_4_BYTES				0x1409
// #define GL_DOUBLE				0x140A

	template <typename T>
	struct format_of;
	template <>
	struct format_of<uint16_t> : mpl::integral_constant<Format, Format::UINT16> {};
	template <>
	struct format_of<uint32_t> : mpl::integral_constant<Format, Format::UINT32> {};
	template <>
	struct format_of<float> : mpl::integral_constant<Format, Format::SINGLE> {};

	constexpr inline std::size_t format_size(const Format format)
	{
		return
			format == Format::UINT16 ? sizeof(uint16_t) :
			format == Format::UINT32 ? sizeof(uint32_t) :
			format == Format::SINGLE ? sizeof(float) :
			throw std::logic_error("format_size failed");
	}
}

#endif /* CORE_FORMAT_HPP */
