
#ifndef CORE_STRINGSERIALIZER_HPP
#define CORE_STRINGSERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/type_traits.hpp"

#include <sstream>
#include <tuple>

namespace core
{
	class StringSerializer
	{
	private:
		std::ostringstream ss;

		std::string string;

	public:
		const char * data() const
		{
			return string.c_str();
		}
		bool empty() const
		{
			return string.empty();
		}
		size_t size() const
		{
			return string.size();
		}

		void clear()
		{
			ss.str("");
		}

		void finalize()
		{
			string = ss.str();
		}

		template <typename ...Ps>
		void write(const std::tuple<Ps...> & x)
		{
			write_tuple(x, mpl::make_index_sequence<sizeof...(Ps)>{});
		}
		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value))>
		void write(const T & x)
		{
			write_class(x);
		}
		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		void write(const T & x)
		{
			write_value(x);
		}
	private:
		template <typename T>
		void write_class(const T & x)
		{
			ss.put('{');
			member_table<T>::call_with_all_members(x, [&](auto && ...ys){ write_list(ys...); });
			ss.put('}');
		}

		void write_list()
		{}
		template <typename T, typename ...Ts>
		void write_list(const T & x, const Ts & ...xs)
		{
			write(x);

			if (sizeof...(Ts) > 0)
			{
				ss.put(',');
				ss.put(' ');
			}
			write_list(xs...);
		}

		template <typename T, std::size_t ...Is>
		void write_tuple(const T & x, mpl::index_sequence<Is...>)
		{
			ss.put('(');
			write_list(std::get<Is>(x)...);
			ss.put(')');
			ss.put('\n');
		}

		template <typename T>
		void write_value(const T & x)
		{
			ss << x;
		}
	};
}

#endif /* CORE_STRINGSERIALIZER_HPP */
