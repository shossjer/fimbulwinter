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
					missing_spaces = 1,
					unexpected_eof,
					unexpected_eol,
					unexpected_symbol,
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
			ext::ssize error(ext::ssize size, ful::unit_utf8 * end)
			{
				static_cast<void>(end);

				error_.where = size;
				error_.type = ful::view_utf8{};

				switch (E)
				{
				case error_code::missing_spaces:
					error_.message = ful::cstr_utf8("missing spaces around =");
					return E;
				case error_code::unexpected_eof:
					error_.message = ful::cstr_utf8("unexpected end of file");
					return E;
				case error_code::unexpected_eol:
					error_.message = ful::cstr_utf8("unexpected end of line");
					return E;
				case error_code::unexpected_symbol:
					error_.message = ful::cstr_utf8("unexpected symbol");
					return E;
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
			ext::ssize error(ext::ssize size, ful::unit_utf8 * end, T & x)
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
				case error_code::type_mismatch:
					error_.message = ful::cstr_utf8("type mismatch");
					return E;
				case error_code::missing_spaces:
				case error_code::unexpected_eof:
				case error_code::unexpected_eol:
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
			ext::ssize read_all(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				if (size >= 0)
					return size;

				// note this special case is needed in order to avoid
				// rundant bounds checking where the separator is actually
				// parsed
				if (*(end + size) == '=')
					return error<error_code::unexpected_symbol>(size, end);

				size = read_key_values(size, end, x, 0);
				if (size >= 0)
					return size;

				while (true)
				{
					ful::view_utf8 header;
					size = read_header(size, end, header);
					if (size >= 0)
						return size;

					const auto member = core::member_table<T>::find(header);
					if (member != std::size_t(-1))
					{
						size = core::member_table<T>::call(member, x, [&](auto & y){ return read_key_values(size, end, y, 0); });
						if (size >= 0)
							return size;
					}
					else
					{
						size = skip_key_values(size, end);
						if (size >= 0)
							return size;
					}
				}
			}

			ext::ssize read_header(ext::ssize size, ful::unit_utf8 * end, ful::view_utf8 & x)
			{
				if (!ful_expect(*(end + size) == '['))
					return error<error_code::unexpected_symbol>(size, end);

				const ful::unit_utf8 * const from = end + size + 1;

				size = skip_to_line(size, end);

				if (*(end + size - 1) != ']')
					return error<error_code::unexpected_eol>(size, end);

				x = ful::view_utf8(from, end + size - 1);

				return size;
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_key_values(ext::ssize size, ful::unit_utf8 * end, T & x, int)
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
						*(end + size) = ';'; // hack
					}
				}

				return size;
			}

			template <typename T>
			ext::ssize read_key_values(ext::ssize size, ful::unit_utf8 * end, T & x, ...)
			{
				return error<error_code::type_mismatch>(size, end, x);
			}

			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, bool & x)
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end);

				// size++; // '='
				// size++; // ' '

				if (size <= -6)
				{
					if (*reinterpret_cast<unsigned int *>(end + size + 6 - 4) == 0x65757274) // 'true' (LE)
					{
						x = true;
						return size + 6;
					}
				}

				if (size <= -7)
				{
					if (*reinterpret_cast<unsigned long long *>(end + size + 7 - 8) == 0x65736c6166203d20ull) // ' = false' (LE)
					{
						x = false;
						return size + 7;
					}
				}

				return error<error_code::unexpected_symbol>(size, end, x);
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(fio::from_chars(end + size + 1, end, x), ext::ssize())
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				size++; // '='
				size++; // ' '

				const ext::ssize n = fio::from_chars(end + size, end, x) - (end + size);
				if (n <= 0)
					return error<error_code::unexpected_symbol>(size, end, x);

				return size + n;
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				const ful::unit_utf8 * const from = end + size + 2;

				size++; // '='
				// size++; // ' '

				size = skip_to_line(size, end);

				const ful::view_utf8 str(from, end + size);

				const auto index = core::value_table<T>::find(str);
				if (index != std::size_t(-1))
				{
					x = core::value_table<T>::get(index);

					return size;
				}
				else
				{
					return error<error_code::unexpected_symbol>(size, end, x);
				}
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
			{
				return error<error_code::type_mismatch>(size, end, x);
			}

			template <typename T>
			auto read_value(ext::ssize size, ful::unit_utf8 * end, T & x)
				-> decltype(x = T(ful::view_utf8{}), ext::ssize())
			{
				if (!ful_expect(*(end + size) == '='))
					return error<error_code::unexpected_symbol>(size, end, x);

				const ful::unit_utf8 * const from = end + size + 2;

				size++; // '='
				// size++; // ' '

				size = skip_to_line(size, end);

				const ful::view_utf8 str(from, end + size);

				x = T(str);

				return size;
			}

			ext::ssize skip_key_values(ext::ssize size, ful::unit_utf8 * end)
			{
				while (true)
				{
					size = skip_whitespace(size, end);
					if (size >= 0)
						return size;

					if (*(end + size) == '[')
						break;

					*(end + size) = ';'; // hack
				}

				return size;
			}

			ful_inline
			ext::ssize skip_to_char(ext::ssize size, ful::unit_utf8 * end, ful::unit_utf8 c)
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
			ext::ssize skip_to_line(ext::ssize size, ful::unit_utf8 * end)
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
			ext::ssize skip_whitespace(ext::ssize size, ful::unit_utf8 * end)
			{
				if (!ful_expect(size < 0))
					return error<error_code::unexpected_eof>(size, end);

				switch (*(end + size))
				{
					do
					{
				case ';':
						size = skip_to_line(size, end);
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
