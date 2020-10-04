#pragma once

#include "utility/string.hpp"

namespace utility
{
	using heap_string_utf8 = basic_string<utility::heap_storage_traits, encoding_utf8>;
//	using heap_string_utf16 = basic_string<utility::heap_storage_traits, encoding_utf16>;
//	using heap_string_utf32 = basic_string<utility::heap_storage_traits, encoding_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using heap_string_utfw = basic_string<utility::heap_storage_traits, encoding_utfw>;
#endif

	template <std::size_t Capacity>
	using static_string_utf8 = basic_string<utility::static_storage_traits<Capacity>, encoding_utf8>;
	//template <std::size_t Capacity>
	//using static_string_utf16 = basic_string<utility::static_storage_traits<Capacity>, encoding_utf16>;
	//template <std::size_t Capacity>
	//using static_string_utf32 = basic_string<utility::static_storage_traits<Capacity>, encoding_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	template <std::size_t Capacity>
	using static_string_utfw = basic_string<utility::static_storage_traits<Capacity>, encoding_utfw>;
#endif

#if defined(_MSC_VER) && defined(_UNICODE)
	// inspired by
	//
	// http://utf8everywhere.org/#how.cvt
	template <typename StorageTraits>
	bool try_narrow(utility::string_points_utfw in, utility::basic_string<StorageTraits, utility::encoding_utf8> & out)
	{
		if (!out.try_resize(in.size() * 2)) // note greedily allocate upper bound
		{
			ext::usize size = 0;

			typename utility::encoding_traits_utf8::buffer_type buffer;
			for (auto cp : in)
			{
				size += cp.get(buffer.data());
			}

			if (!out.try_resize(size))
				return false;
		}

		utility::encoding_traits_utf8::pointer p = out.data();
		for (auto cp : in)
		{
			p += cp.get(p);
		}

		if (!out.try_resize(p - out.data())) // todo no_failure
			return false;

		return true;
	}

	template <typename StorageTraits, typename Encoding>
	auto narrow(utility::string_points_utfw in)
	{
		basic_string<StorageTraits, Encoding> out;

		try_narrow(in, out);

		return out;
	}

	template <std::size_t Capacity, typename Encoding>
	auto static_narrow(utility::string_points_utfw in)
	{
		return narrow<utility::static_storage_traits<Capacity>, Encoding>(in);
	}

	template <typename Encoding>
	auto heap_narrow(utility::string_points_utfw in)
	{
		return narrow<utility::heap_storage_traits, Encoding>(in);
	}

	template <typename StorageTraits>
	bool try_widen(utility::string_points_utf8 in, utility::basic_string<StorageTraits, utility::encoding_utfw> & out)
	{
		const auto no_matching_token_found_left_brace_vs_hack = out.try_resize(in.size());
		if (!no_matching_token_found_left_brace_vs_hack)
		{
			ext::usize size = 0;

			typename utility::encoding_traits_utfw::buffer_type buffer;
			for (auto cp : in)
			{
				size += cp.get(buffer.data());
			}

			if (!out.try_resize(size))
				return false;
		}

		utility::encoding_traits_utfw::pointer p = out.data();
		for (auto cp : in)
		{
			p += cp.get(p);
		}

		if (!out.try_resize(p - out.data())) // todo no_failure
			return false;

		return true;
	}

	template <typename StorageTraits>
	bool try_widen_append(utility::string_points_utf8 in, utility::basic_string<StorageTraits, utility::encoding_utfw> & out)
	{
		const auto offset = out.size();

		if (!out.try_resize(offset + in.size())) // note greedily allocate upper bound
		{
			ext::usize size = 0;

			typename utility::encoding_traits_utfw::buffer_type buffer;
			for (auto cp : in)
			{
				size += cp.get(buffer.data());
			}

			if (!out.try_resize(size))
				return false;
		}

		utility::encoding_traits_utfw::pointer p = out.data() + offset;
		for (auto cp : in)
		{
			p += cp.get(p);
		}

		if (!out.try_resize(p - out.data())) // todo no_failure
			return false;

		return true;
	}

	template <typename StorageTraits>
	auto widen(utility::string_points_utf8 in)
	{
		basic_string<StorageTraits, utility::encoding_utfw> out;

		try_widen(in, out);

		return out;
	}

	template <std::size_t Capacity>
	auto static_widen(utility::string_points_utf8 in)
	{
		return widen<utility::static_storage_traits<Capacity>>(in);
	}

	auto heap_widen(utility::string_points_utf8 in)
	{
		return widen<utility::heap_storage_traits>(in);
	}
#endif
}
