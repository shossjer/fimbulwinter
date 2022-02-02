#pragma once

#include "core/content.hpp"
#include "core/error.hpp"
#include "core/serialization.hpp"

#include "fio/from_chars.hpp"

namespace core
{
	namespace detail
	{
		struct structure_ini
		{
		private:

			struct error_code
			{
				enum type : ext::ssize
				{
					// reached_eof, // not an error
					unexpected_eof = 1,
					unexpected_eol,
					unexpected_header1,
					unexpected_key,
					unexpected_symbol,
					unexpected_value,
					unexpected_value2,
					missing_spaces,
					type_mismatch,
				};
			};

			text_error error_;

			template <error_code::type E>
#if defined(_MSC_VER)
			__declspec(noinline)
#else
			__attribute__((noinline))
#endif
			ext::ssize error(ext::ssize size, const ful::unit_utf8 * end)
			{
				static_cast<void>(end);

				error_.where = size;
				error_.type = ful::view_utf8{};

				switch (E)
				{
				case error_code::unexpected_eof:
					error_.message = ful::cstr_utf8("unexpected end of file");
					return E;
				case error_code::unexpected_eol:
					error_.message = ful::cstr_utf8("unexpected end of line");
					return E;
				case error_code::unexpected_header1:
					error_.where += 1; // '['
					error_.message = ful::cstr_utf8("unexpected header");
					return E;
				case error_code::unexpected_key:
					error_.message = ful::cstr_utf8("unexpected key");
					return E;
				case error_code::unexpected_symbol:
					error_.message = ful::cstr_utf8("unexpected symbol");
					return E;
				case error_code::missing_spaces:
					error_.message = ful::cstr_utf8("missing spaces around =");
					return E;
				case error_code::unexpected_value:
				case error_code::unexpected_value2:
				case error_code::type_mismatch:
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
			ext::ssize error(ext::ssize size, const ful::unit_utf8 * end, T & x)
			{
				static_cast<void>(end);
				static_cast<void>(x);

				constexpr const auto name = utility::type_name<T>();

				error_.where = size;
				error_.type = name;

				switch (E)
				{
				case error_code::unexpected_symbol:
					error_.message = ful::cstr_utf8("unexpected symbol");
					return E;
				case error_code::unexpected_value2:
					error_.where += 2; // '= '
				case error_code::unexpected_value:
					error_.message = ful::cstr_utf8("unexpected value");
					return E;
				case error_code::type_mismatch:
					error_.message = ful::cstr_utf8("type mismatch");
					return E;
				case error_code::unexpected_eof:
				case error_code::unexpected_eol:
				case error_code::unexpected_header1:
				case error_code::unexpected_key:
				case error_code::missing_spaces:
#if defined(_MSC_VER)
				default:
#endif
					ful_unreachable();
				}
			}

		public:

			const text_error & error() const { return error_; }

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_all(ext::ssize size, const ful::unit_utf8 * end, T & x)
			{
				if (size >= 0)
					return size;

				// note this special case is needed in order to avoid
				// rundant bounds checking where the separator is actually
				// parsed
				if (*(end + size) == '=')
					return error<error_code::missing_spaces>(size, end);

				size = read_key_values(size, end, x, 0);
				if (size >= 0)
					return size;

				while (true)
				{
					ful::view_utf8 header;
					const ext::ssize length = read_header(size, end, header);
					if (length >= 0)
						return length;

					const auto member = core::member_table<T>::find(header);
					if (member != std::size_t(-1))
					{
						size = core::member_table<T>::call(member, x, [&](auto & y){ return read_key_values(length, end, y, 0); });
						if (size >= 0)
							return size;
					}
					else
					{
						return error<error_code::unexpected_header1>(size, end);
					}
				}
			}

			ext::ssize read_header(ext::ssize size, const ful::unit_utf8 * end, ful::view_utf8 & x)
			{
				if (!ful_expect(*(end + size) == '['))
					return error<error_code::unexpected_symbol>(size, end);

				const ful::unit_utf8 * const from = end + size + 1;

				size = skip_to_char(size, end, ']');
				if (size >= 0)
					return size;

				size++; // ']'
				if (size < 0)
				{
					if (*(end + size) == '\n' || *(end + size) == '\r')
					{
						x = ful::view_utf8(from, end + size - 1);

						return size;
					}
					else
					{
						return error<error_code::unexpected_symbol>(size, end);
					}
				}

				return error<error_code::unexpected_eof>(size, end);
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_key_values(ext::ssize size, const ful::unit_utf8 * end, T & x, int)
			{
				while (true)
				{
					size = skip_whitespace(size, end);
					if (size >= 0)
						return size;

					if (*(end + size) == '[')
						break;

					const ful::unit_utf8 * const from = end + size;

					if (*(end + size) != '=')
					{
						size = skip_to_char(size, end, '=');
						if (size >= 0)
							return size;
					}

					if (*(end + size - 1) != ' ' || *(end + size + 1) != ' ')
						return error<error_code::missing_spaces>(size, end);

					const ful::view_utf8 key(from, end + size - 1);

					const auto member = core::member_table<T>::find(key);
					if (member != std::size_t(-1))
					{
						size = core::member_table<T>::call(member, x, [&](auto & y){ return read_value(size, end, y); });
						if (size >= 0)
							return size;
					}
					else
					{
						return error<error_code::unexpected_key>(from - end, end);
					}
				}

				return size;
			}

			template <typename T>
			ext::ssize read_key_values(ext::ssize size, const ful::unit_utf8 * end, T & x, ...)
			{
				return error<error_code::type_mismatch>(size, end, x);
			}

			ext::ssize read_value(ext::ssize size, const ful::unit_utf8 * end, bool & x)
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end);

				// size++; // '='
				// size++; // ' '

				if (size <= -(2 + 4 + 1)) // one extra for eol
				{
					if (*reinterpret_cast<const unsigned long long *>(end + size + 2 + 4 + 1 - 8) == 0x0a65757274203d20ull) // ' = true\n' (LE)
					{
						x = true;
						return size + 2 + 4;
					}
					else if (*reinterpret_cast<const unsigned long long *>(end + size + 2 + 4 + 1 - 8) == 0x0d65757274203d20ull) // ' = true\r' (LE)
					{
						x = true;
						return size + 2 + 4;
					}
				}

				if (size <= -(2 + 5 + 1)) // one extra for eol
				{
					if (*reinterpret_cast<const unsigned long long *>(end + size + 2 + 5 + 1 - 8) == 0x0a65736c6166203dull) // '= false\n' (LE)
					{
						x = false;
						return size + 2 + 5;
					}
					else if (*reinterpret_cast<const unsigned long long *>(end + size + 2 + 5 + 1 - 8) == 0x0d65736c6166203dull) // '= false\r' (LE)
					{
						x = false;
						return size + 2 + 5;
					}
				}

				return error<error_code::unexpected_value2>(size, end, x);
			}

			template <typename T>
			auto read_value(ext::ssize size, const ful::unit_utf8 * end, T & x)
				-> decltype(fio::from_chars(end, end, x), ext::ssize())
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				size++; // '='
				size++; // ' '

				const ext::ssize n = (end + size) - fio::from_chars(end + size, end, x);
				if (n < 0)
				{
					if (size < n)
					{
						if (*(end + size - n) == '\n' || *(end + size - n) == '\r')
						{
							return size - n;
						}
					}
				}

				return error<error_code::unexpected_value>(size, end, x);
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			ext::ssize read_value(ext::ssize size, const ful::unit_utf8 * end, T & x)
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				const ful::unit_utf8 * const from = end + size + 2;

				// size++; // '='
				// size++; // ' '

				const ext::ssize length = skip_to_line(size + 1, end);

				const ful::view_utf8 str(from, end + length);

				const auto index = core::value_table<T>::find(str);
				if (index != std::size_t(-1))
				{
					x = core::value_table<T>::get(index);

					return length;
				}
				else
				{
					return error<error_code::unexpected_value2>(size, end, x);
				}
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_value(ext::ssize size, const ful::unit_utf8 * end, T & x)
			{
				return error<error_code::type_mismatch>(size, end, x);
			}

			template <typename T>
			auto read_value(ext::ssize size, const ful::unit_utf8 * end, T & x)
				-> decltype(x = T(ful::view_utf8{}), ext::ssize())
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				const ful::unit_utf8 * const from = end + size + 2;

				// size++; // '='
				// size++; // ' '

				const ext::ssize length = skip_to_line(size + 1, end);

				const ful::view_utf8 str(from, end + length);

				x = T(str);

				return length;
			}

			ful_inline
			ext::ssize skip_to_char(ext::ssize size, const ful::unit_utf8 * end, ful::unit_utf8 c)
			{
				if (!ful_expect(size < 0))
					return error<error_code::unexpected_eof>(size, end);

				do
				{
					do
					{
						size++;
						if (size >= 0)
							return error<error_code::unexpected_eof>(size, end);
					}
					while (static_cast<ful::uint8>(*(end + size)) > c);

					if (*(end + size) == '\r' || *(end + size) == '\n')
						return error<error_code::unexpected_eol>(size, end);
				}
				while (*(end + size) != c);

				return size;
			}

			ful_inline
			ext::ssize skip_to_line(ext::ssize size, const ful::unit_utf8 * end)
			{
				if (!ful_expect(size < 0))
					return error<error_code::unexpected_eof>(size, end);

				do
				{
					do
					{
						size++;
						if (size >= 0)
							return error<error_code::unexpected_eof>(size, end);
					}
					while (static_cast<ful::uint8>(*(end + size)) > '\r');
				}
				while (*(end + size) != '\r' && *(end + size) != '\n');

				return size;
			}

			ful_inline
			ext::ssize skip_whitespace(ext::ssize size, const ful::unit_utf8 * end)
			{
				if (!ful_expect(size < 0))
					return error<error_code::unexpected_eof>(size, end);

				switch (*(end + size))
				{
					do
					{
				case ';':
						size = skip_to_line(size, end);
						if (size >= 0)
							return size;
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
				case 20:
				case 21:
				case 22:
				case 23:
				case 24:
				case 25:
				case 26:
				case 27:
				case 28:
				case 29:
				case 30:
				case 31:
				case 32:
						do
						{
							size++;
							if (size >= 0)
								return size;
						}
						while (static_cast<ful::uint8>(*(end + size)) <= ' ');
					}
					while (*(end + size) == ';');
				}

				return size;
			}
		};
	}

	template <typename T>
	ful_inline
	bool structure_ini(core::content & content, T & x)
	{
		ful::unit_utf8 * const begin = static_cast<ful::unit_utf8 *>(content.data());
		ful::unit_utf8 * const end = static_cast<ful::unit_utf8 *>(content.data()) + content.size();

		detail::structure_ini state;
		const ext::ssize remainder = state.read_all(begin - end, end, x);
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
