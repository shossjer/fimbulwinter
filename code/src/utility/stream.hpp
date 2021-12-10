#pragma once

#include "utility/type_info.hpp"
#include "utility/type_traits.hpp"

#include "fio/stdio.hpp"

namespace mpl
{
	// todo move to fio
	template <typename T, typename = void>
	struct is_ostreamable_impl : mpl::false_type {};
	template <typename T>
	struct is_ostreamable_impl<T, mpl::void_t<decltype(std::declval<fio::stdostream &>() << std::declval<T>())>> : mpl::true_type {};
	template <typename T>
	using is_ostreamable = is_ostreamable_impl<T>;
}

namespace utility
{
	// todo remove
	template <typename Stream>
	bool to_stream(Stream && stream) { static_cast<void>(stream); return true; }
	template <typename Stream, typename T, typename ...Ts>
	bool to_stream(Stream && stream, T && t, Ts && ...ts)
	{
		if (stream << std::forward<T>(t)) return to_stream(stream, std::forward<Ts>(ts)...);

		return false;
	}

	/**
	 */
	template <typename T>
	struct try_stream_yes_t
	{
		T && t;

		try_stream_yes_t(T && t) : t(t) {}

		template <typename Stream>
		friend Stream & operator << (Stream & stream, try_stream_yes_t<T> && t)
		{
			return stream << std::forward<T>(t.t);
		}
	};
	template <typename T>
	struct try_stream_no_t
	{
		template <typename Stream>
		friend Stream & operator << (Stream & stream, try_stream_no_t<T> &&)
		{
			return stream << "?";
		}
	};
	template <typename T,
	          mpl::enable_if_t<mpl::is_ostreamable<T>::value, int> = 0>
	try_stream_yes_t<T> try_stream(T && t)
	{
		return try_stream_yes_t<T>{t};
	}
	template <typename T,
	          mpl::disable_if_t<mpl::is_ostreamable<T>::value, int> = 0>
	try_stream_no_t<T> try_stream(T &&)
	{
		return try_stream_no_t<T>{};
	}
}
