#pragma once

#include "utility/arithmetics.hpp"
#include "utility/type_traits.hpp"

namespace utility
{
	template <typename T>
	using trivial_encoding_type = mpl::negation<std::is_class<T>>;

	template <typename T, bool = trivial_encoding_type<T>::value>
	struct encoding_traits_impl
	{
		using code_unit = T;
		using code_point = T;

		using difference_type = std::ptrdiff_t;

		static constexpr code_point & dereference(code_unit * p) { return *p; }
		static constexpr const code_point & dereference(const code_unit * p) { return *p; }

		template <typename Char>
		static constexpr std::size_t size(code_point) { return 1; }
		static constexpr std::size_t max_size() { return 1; }

		template <typename Char>
		static constexpr auto get(code_point cp, Char * buffer) -> decltype(*buffer = cp, std::ptrdiff_t()) { *buffer = cp; return 1; }

		static constexpr std::ptrdiff_t next(const code_unit *) { return 1; }
		static constexpr std::ptrdiff_t next(const code_unit *, std::ptrdiff_t count) { return count; }

		static constexpr std::ptrdiff_t previous(const code_unit *) { return 1; }
		static constexpr std::ptrdiff_t previous(const code_unit *, std::ptrdiff_t count) { return count; }

		static constexpr std::ptrdiff_t count(const code_unit * begin, const code_unit * end) { return end - begin; }

		static constexpr difference_type difference(const code_unit * begin, const code_unit * end) { return end - begin; }
	};
	template <typename T>
	struct encoding_traits_impl<T, false /*trivial encoding type*/>
	{
		using code_unit = typename T::code_unit;
		using code_point = typename T::code_point;

		using difference_type = typename T::difference_type;

		static constexpr code_point dereference(const code_unit * p) { return code_point(p); } // todo T::dereference

		template <typename Char>
		static constexpr std::size_t size(code_point cp) { return cp.template size<Char>(); } // todo T::size
		static constexpr std::size_t max_size() { return T::max_size(); }

		template <typename Char>
		static constexpr std::ptrdiff_t get(code_point cp, Char * buffer) { return cp.get(buffer); } // todo T::get

		static constexpr std::ptrdiff_t next(const code_unit * s) { return code_point::next(s); } // todo T::next
		template <typename Count>
		static constexpr std::ptrdiff_t next(const code_unit * s, Count && count) { return code_point::next(s, std::forward<Count>(count)); } // todo T::next

		static constexpr std::ptrdiff_t previous(const code_unit * s) { return code_point::previous(s); } // todo T::previous
		template <typename Count>
		static constexpr std::ptrdiff_t previous(const code_unit * s, Count && count) { return code_point::previous(s, std::forward<Count>(count)); } // todo T::previous

		static constexpr std::ptrdiff_t count(const code_unit * begin, const code_unit * end) { return code_point::count(begin, end); } // todo T::count

		static constexpr difference_type difference(const code_unit * begin, const code_unit * end) { return T::difference(begin, end); }
	};

	template <typename T>
	using encoding_traits = encoding_traits_impl<T>;
}
