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

		static constexpr code_point & dereference(code_unit * p) { return *p; }
		static constexpr const code_point & dereference(const code_unit * p) { return *p; }

		static constexpr std::size_t max_size() { return 1; }

		static constexpr std::ptrdiff_t get(code_point cp, code_unit * buffer) { *buffer = cp; return 1; }

		static constexpr std::ptrdiff_t next(const code_unit *) { return 1; }
		static constexpr std::ptrdiff_t next(const code_unit *, std::ptrdiff_t count) { return count; }

		static constexpr std::ptrdiff_t previous(const code_unit *) { return 1; }
		static constexpr std::ptrdiff_t previous(const code_unit *, std::ptrdiff_t count) { return count; }

		static constexpr std::ptrdiff_t count(const code_unit * begin, const code_unit * end) { return end - begin; }
	};
	template <typename T>
	struct encoding_traits_impl<T, false /*trivial encoding type*/>
	{
		using code_unit = typename T::code_unit;
		using code_point = typename T::code_point;

		static constexpr code_point dereference(const code_unit * p) { return code_point(p); }

		static constexpr std::size_t max_size() { return T::max_size(); }

		static constexpr std::ptrdiff_t  get(code_point cp, code_unit * buffer) { return cp.get(buffer); }

		static constexpr std::ptrdiff_t next(const code_unit * s) { return code_point::next(s); }
		template <typename Count>
		static constexpr std::ptrdiff_t next(const code_unit * s, Count && count) { return code_point::next(s, std::forward<Count>(count)); }

		static constexpr std::ptrdiff_t previous(const code_unit * s) { return code_point::previous(s); }
		template <typename Count>
		static constexpr std::ptrdiff_t previous(const code_unit * s, Count && count) { return code_point::previous(s, std::forward<Count>(count)); }

		static constexpr std::ptrdiff_t count(const code_unit * begin, const code_unit * end) { return code_point::count(begin, end); }
	};

	template <typename T>
	using encoding_traits = encoding_traits_impl<T>;

	struct point_difference : Arithmetic<point_difference, std::ptrdiff_t>
	{
		using Arithmetic<point_difference, std::ptrdiff_t>::Arithmetic;
	};

	struct unit_difference : Arithmetic<unit_difference, std::ptrdiff_t>
	{
		using Arithmetic<unit_difference, std::ptrdiff_t>::Arithmetic;
	};

	template <typename Encoding>
	class lazy_difference
	{
		using encoding_traits = encoding_traits<Encoding>;

		using code_unit = typename encoding_traits::code_unit;
	private:
		const code_unit * begin_;
		const code_unit * end_;

	public:
		constexpr lazy_difference(const code_unit * begin, const code_unit * end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr operator point_difference () const
		{
			if (end_ < begin_)
				return point_difference(-encoding_traits::count(end_, begin_));
			return point_difference(encoding_traits::count(begin_, end_));
		}
		constexpr operator unit_difference () const { return unit_difference(end_ - begin_); }
	};
}
