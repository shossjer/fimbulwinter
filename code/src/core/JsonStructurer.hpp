#pragma once

#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/algorithm.hpp"

#include "ful/cstrext.hpp"
#include "ful/string_search.hpp"

namespace core
{
	namespace detail
	{
		template <typename T>
		auto grow_range(T & x, int)
			-> decltype(x.emplace_back() == x.data(), x.data())
		{
			return x.emplace_back();
		}

		// todo iff exceptions enabled
		template <typename T>
		auto grow_range(T & x, int)
			-> decltype(&x.emplace_back() == x.data(), x.data())
		{
			//std::is_nothrow_default_constructible<typename T::value_type>::value
			return &x.emplace_back();
		}

		// todo iff exceptions enabled
		template <typename T>
		auto grow_range(T & x, ...)
			-> decltype(x.emplace_back(), x.back(), x.data())
		{
			//std::is_nothrow_default_constructible<typename T::value_type>::value
			x.emplace_back();
			return &x.back();
		}
	}

	template <typename T>
	auto grow_range(T & x)
		-> decltype(detail::grow_range(x, 0))
	{
		return detail::grow_range(x, 0);
	}

	namespace detail
	{
		struct structure_json
		{
			struct error_json
			{
				enum type
				{
					unexpected_eof,
					unexpected_error,
					unexpected_symbol,
				};

				ful::view_utf8 type_name;
			}
			err;

			template <error_json::type E>
#if defined(_MSC_VER)
			__declspec(noinline)
#else
			__attribute__((noinline))
#endif
			ext::ssize structure_json_error(ext::ssize size, ful::unit_utf8 * end)
			{
				static_cast<void>(end);

				err.type_name = ful::view_utf8{};

				return -size;
			}

			template <error_json::type E, typename T>
#if defined(_MSC_VER)
			__declspec(noinline)
#else
			__attribute__((noinline))
#endif
			ext::ssize structure_json_error(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				static_cast<void>(end);
				static_cast<void>(x);

				constexpr const auto type_name = utility::type_name<T>();
				err.type_name = type_name;

				return -size;
			}

			inline
			ext::ssize structure_json_whitespace(ext::ssize size, ful::unit_utf8 * end)
			{
				if (size < 0)
				{
					do
					{
						if (static_cast<unsigned char>(*(end + size)) > 32)
							break;

						size++;
					}
					while (size < 0);
				}
				return size;
			}

			inline
			ext::ssize structure_json_string(ext::ssize size, ful::unit_utf8 * end, ful::view_utf8 & x)
			{
				if (!debug_inform(*(end + size) == '"'))
					return structure_json_error<error_json::unexpected_symbol>(size, end);

				const ext::ssize first = size + 1;

				while (true)
				{
					size++;

					ful::unit_utf8 * found = ful::find(end + size, end, ful::char8{'"'});
					if (found == end)
						return structure_json_error<error_json::unexpected_eof>(size, end);

					size = found - end;

					if (*(found - 1) != '\\')
						break;
					if (*(found - 2) == '\\')
					{
						if (*(found - 3) != '\\')
							break;
						if (*(found - 4) == '\\')
						{
							if (!debug_fail("more than 3 \\ at the end of string not implemented"))
								return structure_json_error<error_json::unexpected_error>(size, end);
						}
					}
				}

				const ext::ssize last = size;
				x = ful::view_utf8(end + first, end + last);

				return size;
			}

			inline
			ext::ssize structure_json_skip(ext::ssize size, ful::unit_utf8 * end)
			{
				switch (*(end + size))
				{
				case '"':
				{
					ful::view_utf8 x;
					size = structure_json_string(size, end, x);
					if (size >= 0)
						return size;

					return size + 1;
				}
				case '-':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
					const ful::unit_utf8 * first; // unused
					const ful::unit_utf8 * dot; // unused
					const ful::unit_utf8 * last; // unused
					fio::ssize exp; // unused
					const ful::unit_utf8 * const ptr = fio::detail::float_parse(end + size, end, first, dot, last, exp);
					if (!debug_inform(ptr != nullptr))
						return structure_json_error<error_json::unexpected_symbol>(size, end);

					return ptr - end;
				}
				case '[':
					size++;

					size = structure_json_whitespace(size, end);
					if (size >= 0)
						return structure_json_error<error_json::unexpected_eof>(size, end);

					if (*(end + size) != ']')
						goto entry_array;

					return size + 1;

					do
					{
						size++;

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end);

					entry_array:
						size = structure_json_skip(size, end);

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end);
					}
					while (*(end + size) == ',');

					if (!debug_inform(*(end + size) == ']'))
						return structure_json_error<error_json::unexpected_symbol>(size, end);

					return size + 1;
				case 'f':
					return size + 5;
				case 'n':
				case 't':
					return size + 4;
				case '{':
					do
					{
						size++;

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return size;

						ful::view_utf8 x;
						size = structure_json_string(size, end, x);
						if (size >= 0)
							return size;

						size++;

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end);

						if (!debug_inform(*(end + size) == ':'))
							return structure_json_error<error_json::unexpected_symbol>(size, end);

						size++;

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end);

						size = structure_json_skip(size, end);

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end);
					}
					while (*(end + size) == ',');

					if (!debug_inform(*(end + size) == '}'))
						return structure_json_error<error_json::unexpected_symbol>(size, end);

					return size + 1;
				default:
					return structure_json_error<error_json::unexpected_symbol>(size, end);
				}
			}

			inline
			ext::ssize structure_json_value(ext::ssize size, ful::unit_utf8 * end, bool & x)
			{
				size = structure_json_whitespace(size, end);

				if (size < -4)
				{
					if (*reinterpret_cast<unsigned int *>(end + size) == 0x736c6166 && // fals (LE)
						*(end + size + 4) == 'e') // e
					{
						x = false;
						return size + 5;
					}
				}
				else if (size > -4)
				{
					return structure_json_error<error_json::unexpected_eof>(size, end, x);
				}

				if (*reinterpret_cast<unsigned int *>(end + size) == 0x65757274) // true (LE)
				{
					x = true;
					return size + 4;
				}
				else
				{
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);
				}
			}

			template <typename T>
			inline
			auto structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(fio::from_chars(end, end, x), ext::ssize())
			{
				size = structure_json_whitespace(size, end);

				const ext::ssize n = fio::from_chars(end + size, end, x) - (end + size);
				if (!debug_inform(n > 0))
					return structure_json_error<error_json::unexpected_symbol>(size, end);

				return size + n;
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			inline
			ext::ssize structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return size;

				ful::view_utf8 str;
				size = structure_json_string(size, end, str);
				if (size >= 0)
					return size;

				const auto index = core::value_table<T>::find(str);
				if (index != std::size_t(-1))
				{
					x = core::value_table<T>::get(index);

					return size + 1;
				}
				else
				{
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);
				}
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			inline
			ext::ssize structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return size;

				if (!debug_inform(*(end + size) == '{'))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				do
				{
					size++;

					size = structure_json_whitespace(size, end);
					if (size >= 0)
						return size;

					ful::view_utf8 key;
					size = structure_json_string(size, end, key);
					if (size >= 0)
						return size;

					size++;

					size = structure_json_whitespace(size, end);
					if (size >= 0)
						return structure_json_error<error_json::unexpected_eof>(size, end, x);

					if (!debug_inform(*(end + size) == ':'))
						return structure_json_error<error_json::unexpected_symbol>(size, end, x);

					size++;

					const auto key_index = core::member_table<T>::find(key);
					if (key_index != std::size_t(-1))
					{
						size = core::member_table<T>::call(key_index, x, [=](auto && y){ return structure_json_value(size, end, static_cast<decltype(y)>(y)); });

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end, x);
					}
					else
					{
						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end, x);

						size = structure_json_skip(size, end);

						size = structure_json_whitespace(size, end);
						if (size >= 0)
							return structure_json_error<error_json::unexpected_eof>(size, end, x);
					}
				}
				while (*(end + size) == ',');

				if (!debug_inform(*(end + size) == '}'))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				return size + 1;
			}

			template <typename T>
			inline
			auto structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(core::grow_range(x), ext::ssize())
			{
				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return size;

				if (!debug_inform(*(end + size) == '['))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				size++;

				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return structure_json_error<error_json::unexpected_eof>(size, end);

				if (*(end + size) != ']')
					goto entry_array;

				return size + 1;

				do
				{
					size++;

				entry_array:
					auto it = core::grow_range(x);
					if (!it)
						return structure_json_error<error_json::unexpected_error>(size, end, x);

					size = structure_json_value(size, end, *it);

					size = structure_json_whitespace(size, end);
					if (size >= 0)
						return structure_json_error<error_json::unexpected_eof>(size, end, x);
				}
				while (*(end + size) == ',');

				if (!debug_inform(*(end + size) == ']'))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				return size + 1;
			}

			template <typename T>
			fio_inline
			ext::ssize structure_json_value_tuple_impl(ext::ssize size, ful::unit_utf8 * end, T & x, mpl::index_constant<(ext::tuple_size<T>::value - 1)>, int)
			{
				size++;

				using ext::get;
				size = structure_json_value(size, end, get<(ext::tuple_size<T>::value - 1)>(x));

				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return structure_json_error<error_json::unexpected_eof>(size, end, x);

				if (!debug_inform(*(end + size) == ']'))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				return size + 1;
			}

			template <typename T, size_t I>
			fio_inline
			ext::ssize structure_json_value_tuple_impl(ext::ssize size, ful::unit_utf8 * end, T & x, mpl::index_constant<I>, float)
			{
				size++;

				using ext::get;
				size = structure_json_value(size, end, get<I>(x));

				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return structure_json_error<error_json::unexpected_eof>(size, end, x);

				if (!debug_inform(*(end + size) == ','))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				return structure_json_value_tuple_impl(size, end, x, mpl::index_constant<(I + 1)>{}, 0);
			}

			template <typename T>
			inline
			auto structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(mpl::enable_if_t<ext::is_tuple<T>::value>(), ext::ssize())
			{
				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return size;

				if (!debug_inform(*(end + size) == '['))
					return structure_json_error<error_json::unexpected_symbol>(size, end, x);

				return structure_json_value_tuple_impl(size, end, x, mpl::index_constant<0>{}, 0);
			}

			template <typename T>
			inline
			auto structure_json_value(ext::ssize size, ful::unit_utf8 * end, T && x)
				-> decltype(sizeof(typename T::proxy_type), ext::ssize())
			{
				typename T::proxy_type proxy;
				size = structure_json_value(size, end, proxy);
				if (size >= 0)
					return size;

				if (!x.set(proxy))
					return structure_json_error<error_json::unexpected_error>(size, end, x);

				return size;
			}

			template <typename T>
			inline
			auto structure_json_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(x = T(ful::view_utf8{}), ext::ssize())
			{
				size = structure_json_whitespace(size, end);
				if (size >= 0)
					return size;

				ful::view_utf8 str;
				size = structure_json_string(size, end, str);
				if (size >= 0)
					return size;

				x = T(str);

				return size + 1;
			}

		};
	}

	template <typename T>
	inline
	ful::unit_utf8 * structure_json(ful::unit_utf8 * begin, ful::unit_utf8 * end, T & x)
	{
		detail::structure_json state;
		const ext::ssize remainder = state.structure_json_value(begin - end, end, x);
		if (remainder <= 0)
		{
			return end + remainder;
		}
		else
		{
			if (ful::empty(state.err.type_name))
			{
				debug_printline("(", (end - remainder - begin), ") error");
			}
			else
			{
				debug_printline("(", (end - remainder - begin), ") error when structuring ", state.err.type_name);
			}

			return nullptr;
		}
	}

	template <typename T>
	inline
	bool structure_json(core::content & content, T & x)
	{
		return core::structure_json(static_cast<ful::unit_utf8 *>(content.data()), static_cast<ful::unit_utf8 *>(content.data()) + content.size(), x) != nullptr;
	}
}
