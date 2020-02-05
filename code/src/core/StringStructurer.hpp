
#ifndef CORE_STRINGSTRUCTURER_HPP
#define CORE_STRINGSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/type_traits.hpp"

#include <sstream>
#include <tuple>

namespace core
{
	class StringStructurer
	{
	private:
		std::istringstream ss;

	public:
		template <typename ...Ps>
		void read(std::tuple<Ps...> & x)
		{
			read_tuple(x, mpl::make_index_sequence<sizeof...(Ps)>{});
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value))>
		void read(T & x)
		{
			read_class(x);
		}
		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		void read(T & x)
		{
			read_value(x);
		}

		template <typename ...Ts>
		void read_as_tuple(Ts && ...xs)
		{
			skip_whitespace();
			debug_verify(ss.get() == '(');
			read_list(std::forward<Ts>(xs)...);
			skip_whitespace();
			debug_verify(ss.get() == ')');
		}
	private:
		template <typename T>
		void read_class(T & x)
		{
			skip_whitespace();
			debug_verify(ss.get() == '{');
			member_table<T>::call_with_all_members(x, [&](auto & ...ys){ read_list(ys...); });
			skip_whitespace();
			debug_verify(ss.get() == '}');
		}

		template <typename T>
		void call_or_read(T && x)
		{
			call_or_read_impl(std::forward<T>(x), 0);
		}

		template <typename T>
		auto call_or_read_impl(T && x, int) -> decltype(x(*this), void())
		{
			x(*this);
		}
		template <typename T>
		auto call_or_read_impl(T && x, ...) -> void
		{
			read(std::forward<T>(x));
		}

		void read_list()
		{}
		template <typename T, typename ...Ts>
		void read_list(T && x, Ts && ...xs)
		{
			call_or_read(std::forward<T>(x));

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4127 )
			// C4127 - conditional expression is constant
#endif
			if (sizeof...(Ts) > 0)
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
			{
				skip_whitespace();
				if (ss.peek() == ')')
					return;
				debug_verify(ss.get() == ',');
			}
			read_list(std::forward<Ts>(xs)...);
		}

		template <typename T, std::size_t ...Is>
		void read_tuple(T & x, mpl::index_sequence<Is...>)
		{
			read_as_tuple(std::get<Is>(x)...);
		}

		template <typename T>
		void read_value(T & x)
		{
			skip_whitespace();
			ss >> x;
		}

		void skip_whitespace()
		{
			for (int c = ss.peek(); c == ' ' || c == '\n'; ss.get(), c = ss.peek());
		}

	public:
		void set(const char * data, size_t size)
		{
			ss.str(std::string(data, size));
		}
	};
}

#endif /* CORE_STRINGSTRUCTURER_HPP */
