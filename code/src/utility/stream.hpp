#pragma once

#include "utility/type_info.hpp"
#include "utility/type_traits.hpp"

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

	namespace detail
	{
		template <typename Stream, typename T>
		auto try_stream_impl(Stream && stream, T && t, int)
			-> decltype(std::forward<Stream>(stream) << std::forward<T>(t))
		{
			return std::forward<Stream>(stream) << std::forward<T>(t);
		}

		template <typename Stream, typename T>
		auto try_stream_impl(Stream && stream, T && t, ...)
			-> decltype(stream)
		{
			static_cast<void>(t);
			return std::forward<Stream>(stream) << static_cast<typename mpl::remove_cvref_t<Stream>::char_type>('?');
		}

		template <typename T>
		struct try_stream_type
		{
			T && t;

			template <typename Stream>
			friend auto operator << (Stream && stream, try_stream_type<T> && x)
				-> decltype(try_stream_impl(std::forward<Stream>(stream), std::forward<T>(x.t), 0))
			{
				return try_stream_impl(std::forward<Stream>(stream), std::forward<T>(x.t), 0);
			}
		};
	}

	template <typename T>
	detail::try_stream_type<T> try_stream(T && t)
	{
		return detail::try_stream_type<T>{std::forward<T>(t)};
	}
}
