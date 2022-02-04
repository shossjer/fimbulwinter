#pragma once

#include "core/content.hpp"
#include "core/error.hpp"
#include "core/serialization.hpp"

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
		private:

			struct error_code
			{
				enum type : ext::ssize
				{
					// reached_eof, // not an error
					unexpected_eof = 1,
					unexpected_error,
					unexpected_key,
					unexpected_symbol,
					unexpected_value,
					expected_array,
					expected_less,
					expected_more,
					expected_object,
					expected_string,
				};
			};

			text_error error_;

			template <error_code::type E>
#if defined(_MSC_VER)
			__declspec(noinline)
#else
			__attribute__((noinline))
#endif
			ext::ssize error(ext::ssize size, ful::unit_utf8 * end)
			{
				static_cast<void>(end);

				error_.where = size;
				error_.type = ful::view_utf8{};

				switch (E)
				{
				case error_code::unexpected_eof:
					error_.message = ful::cstr_utf8("unexpected end of file");
					return E;
				case error_code::unexpected_error:
					error_.message = ful::cstr_utf8("unexpected error");
					return E;
				case error_code::unexpected_key:
					error_.message = ful::cstr_utf8("unexpected key");
					return E;
				case error_code::unexpected_symbol:
					error_.message = ful::cstr_utf8("unexpected symbol");
					return E;
				case error_code::expected_array:
					error_.message = ful::cstr_utf8("expected array");
					return E;
				case error_code::expected_less:
					error_.message = ful::cstr_utf8("expected less");
					return E;
				case error_code::expected_more:
					error_.message = ful::cstr_utf8("expected more");
					return E;
				case error_code::expected_object:
					error_.message = ful::cstr_utf8("expected object");
					return E;
				case error_code::expected_string:
					error_.message = ful::cstr_utf8("expected string");
					return E;
				case error_code::unexpected_value:
#if defined(_MSC_VER)
				default:
#endif
					ful_unreachable();
				}
			}

			template <error_code::type E, typename T>
#if defined(_MSC_VER)
			__declspec(noinline)
#else
			__attribute__((noinline))
#endif
			ext::ssize error(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				static_cast<void>(end);
				static_cast<void>(x);

				constexpr const auto name = utility::type_name<T>();

				error_.where = size;
				error_.type = name;

				switch (E)
				{
				case error_code::unexpected_value:
					error_.message = ful::cstr_utf8("unexpected value");
					return E;
				case error_code::unexpected_eof:
				case error_code::unexpected_error:
				case error_code::unexpected_key:
				case error_code::unexpected_symbol:
				case error_code::expected_array:
				case error_code::expected_less:
				case error_code::expected_more:
				case error_code::expected_object:
				case error_code::expected_string:
#if defined(_MSC_VER)
				default:
#endif
					ful_unreachable();
				}
			}

		public:

			const text_error & error() const { return error_; }

			ext::ssize skip_whitespace(ext::ssize size, ful::unit_utf8 * end)
			{
				if (size >= 0)
					return error<error_code::unexpected_eof>(size, end);

				if (static_cast<unsigned char>(*(end + size)) <= ' ')
				{
					do
					{
						size++;
						if (size >= 0)
							return error<error_code::unexpected_eof>(size, end);
					}
					while (static_cast<unsigned char>(*(end + size)) <= ' ');
				}

				return size;
			}

			ext::ssize read_string(ext::ssize size, ful::unit_utf8 * end, ful::view_utf8 & x)
			{
				if (*(end + size) != '"')
					return error<error_code::expected_string>(size, end);

				const ext::ssize first = size + 1;

				while (true)
				{
					size++;

					ful::unit_utf8 * found = ful::find(end + size, end, ful::char8{'"'});
					if (found == end)
						return error<error_code::unexpected_eof>(size, end);

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
								return error<error_code::unexpected_error>(size, end);
						}
					}
				}

				const ext::ssize last = size;
				x = ful::view_utf8(end + first, end + last);

				return size;
			}

			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, bool & x)
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (size <= -4)
				{
					if (*reinterpret_cast<const unsigned int *>(end + size + 4 - 4) == 0x65757274ull) // 'true' (LE)
					{
						x = true;
						return size + 4;
					}
				}

				if (size <= -5)
				{
					if (*reinterpret_cast<const unsigned int *>(end + size + 4 - 4) == 0x736c6166ull && // 'fals' (LE)
						*reinterpret_cast<const unsigned char *>(end + size + 5 - 1) == 'e')
					{
						x = false;
						return size + 5;
					}
				}

				return error<error_code::unexpected_value>(size, end, x);
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(fio::from_chars(end, end, x), ext::ssize())
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				const ext::ssize n = fio::from_chars(end + size, end, x) - (end + size);
				if (n > 0)
				{
					return size + n;
				}
				else
				{
					return error<error_code::unexpected_value>(size, end, x);
				}
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				ful::view_utf8 str;
				ext::ssize length = read_string(size, end, str);
				if (length >= 0)
					return length;

				const auto index = core::value_table<T>::find(str);
				if (index != std::size_t(-1))
				{
					x = core::value_table<T>::get(index);

					return length + 1;
				}
				else
				{
					return error<error_code::unexpected_value>(size, end, x);
				}
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != '{')
					return error<error_code::expected_object>(size, end);

				do
				{
					size++; // '{' or ','

					size = skip_whitespace(size, end);
					if (size >= 0)
						return size;

					ful::view_utf8 key;
					ext::ssize length = read_string(size, end, key);
					if (length >= 0)
						return length;

					const auto key_index = core::member_table<T>::find(key);
					if (key_index != std::size_t(-1))
					{
						length++; // '"'

						size = skip_whitespace(length, end);
						if (size >= 0)
							return size;

						if (*(end + size) != ':')
							return error<error_code::unexpected_symbol>(size, end);

						size++; // ':'

						size = core::member_table<T>::call(key_index, x, [=](auto && y){ return read_value(size, end, static_cast<decltype(y)>(y)); });
						if (size >= 0)
							return size;

						size = skip_whitespace(size, end);
						if (size >= 0)
							return size;
					}
					else
					{
						return error<error_code::unexpected_key>(size, end);
					}
				}
				while (*(end + size) == ',');

				if (*(end + size) != '}')
					return error<error_code::unexpected_symbol>(size, end);

				return size + 1;
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(core::grow_range(x), ext::ssize())
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != '[')
					return error<error_code::expected_array>(size, end);

				size++; // '['

				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != ']')
					goto entry_array;

				return size + 1;

				do
				{
					size++;

				entry_array:
					auto it = core::grow_range(x);
					if (!it)
						return error<error_code::unexpected_error>(size, end);

					size = read_value(size, end, *it);
					if (size >= 0)
						return size;

					size = skip_whitespace(size, end);
					if (size >= 0)
						return size;
				}
				while (*(end + size) == ',');

				if (*(end + size) != ']')
					return error<error_code::unexpected_symbol>(size, end);

				return size + 1;
			}

			template <typename T>
			fio_inline
			ext::ssize read_value_tuple_impl(ext::ssize size, ful::unit_utf8 * end, T & x, mpl::index_constant<(ext::tuple_size<T>::value - 1)>, int)
			{
				size++;

				using ext::get;
				size = read_value(size, end, get<(ext::tuple_size<T>::value - 1)>(x));
				if (size >= 0)
					return size;

				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != ']')
					return error<error_code::expected_less>(size, end);

				return size + 1;
			}

			template <typename T, size_t I>
			fio_inline
			ext::ssize read_value_tuple_impl(ext::ssize size, ful::unit_utf8 * end, T & x, mpl::index_constant<I>, float)
			{
				size++;

				using ext::get;
				size = read_value(size, end, get<I>(x));
				if (size >= 0)
					return size;

				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != ',')
					return error<error_code::expected_more>(size, end);

				return read_value_tuple_impl(size, end, x, mpl::index_constant<(I + 1)>{}, 0);
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(mpl::enable_if_t<ext::is_tuple<T>::value>(), ext::ssize())
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				if (*(end + size) != '[')
					return error<error_code::expected_array>(size, end);

				return read_value_tuple_impl(size, end, x, mpl::index_constant<0>{}, 0);
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T && x)
				-> decltype(sizeof(typename T::proxy_type), ext::ssize())
			{
				typename T::proxy_type proxy;
				size = read_value(size, end, proxy);
				if (size >= 0)
					return size;

				if (!x.set(proxy))
					return error<error_code::unexpected_error>(size, end);

				return size;
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(x = T(ful::view_utf8{}), ext::ssize())
			{
				size = skip_whitespace(size, end);
				if (size >= 0)
					return size;

				ful::view_utf8 str;
				size = read_string(size, end, str);
				if (size >= 0)
					return size;

				x = T(str);

				return size + 1;
			}

		};
	}

	template <typename T>
	ful_inline
	bool structure_json(core::content & content, T & x)
	{
		ful::unit_utf8 * const begin = static_cast<ful::unit_utf8 *>(content.data());
		ful::unit_utf8 * const end = static_cast<ful::unit_utf8 *>(content.data()) + content.size();

		detail::structure_json state;
		const ext::ssize remainder = state.read_value(begin - end, end, x);
		if (remainder <= 0)
		{
			return true;
		}
		else
		{
			report_text_error(content, state.error());

			return false;
		}
	}
}
