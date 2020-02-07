#pragma once

#include "utility/encoding_traits.hpp"
#include "utility/string.hpp"

namespace utility
{
	template <typename EncodingOut, typename EncodingIn>
	constexpr auto size(utility::basic_string_view<EncodingIn> in)
		-> decltype(utility::encoding_traits<EncodingIn>::template size<typename utility::encoding_traits<EncodingOut>::code_unit>(std::declval<typename utility::encoding_traits<EncodingIn>::code_point>()), std::size_t())
	{
		std::ptrdiff_t s = 0;
		for (auto cp : in)
		{
			s += utility::encoding_traits<EncodingIn>::template size<typename utility::encoding_traits<EncodingOut>::code_unit>(cp);
		}
		return s;
	}

	template <typename Encoding, typename Char>
	constexpr auto convert(utility::basic_string_view<Encoding> in, Char * out_data, std::size_t out_length)
		-> decltype(utility::encoding_traits<Encoding>::get(std::declval<typename utility::encoding_traits<Encoding>::code_point>(), out_data), std::ptrdiff_t())
	{
		std::ptrdiff_t p = 0;
		for (auto cp : in)
		{
			p += utility::encoding_traits<Encoding>::get(cp, out_data + p);
			assert(std::size_t(p) <= out_length);
			static_cast<void>(out_length);
		}
		return p;
	}
}
