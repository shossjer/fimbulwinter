#pragma once

#include "utility/ext/stddef.hpp"
#include "utility/type_traits.hpp"

namespace utility
{
	template <typename T>
	using trivial_encoding_type = mpl::negation<std::is_class<T>>;

	template <typename Encoding>
	struct encoding_traits
	{
		static_assert(trivial_encoding_type<Encoding>::value, "Unknown encoding type, did you forget to specialize `utility::encoding_traits`?");

		using encoding_type = Encoding;

		using value_type = Encoding;
		using pointer = Encoding *;
		using const_pointer = const Encoding *;
		using reference = Encoding &;
		using const_reference = const Encoding &;
		using size_type = ext::usize;
		using difference_type = ext::pdiff;

		using buffer_type = std::array<Encoding, 1>;

		static constexpr ext::usize get(Encoding code, Encoding * buffer) { return buffer[0] = code; return 1; }
	};

	template <typename Encoding>
	struct boundary_unit
	{
		static_assert(trivial_encoding_type<Encoding>::value, "Unknown encoding type, did you forget to specialize `utility::boundary_unit`?");

		using encoding_type = Encoding;

		using value_type = typename encoding_traits<Encoding>::value_type;
		using pointer = typename encoding_traits<Encoding>::pointer;
		using const_pointer = typename encoding_traits<Encoding>::const_pointer;
		using reference = typename encoding_traits<Encoding>::reference;
		using const_reference = typename encoding_traits<Encoding>::const_reference;
		using size_type = typename encoding_traits<Encoding>::size_type;
		using difference_type = typename encoding_traits<Encoding>::difference_type;

		static constexpr reference dereference(pointer p) { return *p; }
		static constexpr const_reference dereference(const_pointer p) { return *p; }

		static constexpr ext::ssize next(const_pointer, ext::ssize count = 1) { return count; }

		static constexpr ext::ssize previous(const_pointer, ext::ssize count = 1) { return count; }

		static constexpr ext::usize length(const_pointer begin, const_pointer end) { return end - begin; }
	};

	template <typename Encoding>
	struct boundary_point;
}
