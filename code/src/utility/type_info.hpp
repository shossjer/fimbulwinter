#pragma once

#include "utility/crypto/crc.hpp"
#if !defined(_MSC_VER)
# include "utility/ranges.hpp"
#endif
#include "utility/unicode/string_view.hpp"

#include <cstdint>

namespace utility
{
	namespace detail
	{
		// shamelessly stolen from :imp:
		//
		// https://stackoverflow.com/a/56600402
		template <typename T>
		constexpr const char * get_function_signature()
		{
#if defined(__GNUG__)
			return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
			return __FUNCSIG__;
#else
# error "compiler does not support constexpr type information"
#endif
		}

		// ditto
		template <typename T>
		constexpr string_units_utf8 get_type_signature()
		{
			string_units_utf8 signature = get_function_signature<T>();

#if defined(__GNUG__)
			const auto from = find(signature, "T = ") + 4;
			const auto to = rfind(signature, "]");
#elif defined(_MSC_VER)
			const auto from = find(signature, "utility::detail::get_function_signature<") + 40;
			const auto to = rfind(signature, ">(");
#endif

			return string_units_utf8(from, to);
		}

		// in order to make the names platform independent (to the best of our
		// ability), we substitute certain patterns, however, constructing
		// strings at compile time is awkward, but this 2-stage approach of
		// first computing the length of the platform independent name and then
		// allocate an array of that size and store the substitutions in there
		// seem to work pretty okay, although it needs c++14

		inline constexpr bool is_identifier_character(const char c)
		{
			// this should probably work with unicode as well
			return !(
				(0 <= c && c < '0') ||
				('9' < c && c < 'A') ||
				('Z' < c && c < '_') ||
				('_' < c && c < 'a') ||
				('z' < c && c <= 127)
				);
		}

		struct TypePattern
		{
			std::size_t replace;
			string_units_utf8 with;
		};

		inline constexpr TypePattern next_word(const char * const cstr)
		{
			std::size_t count = 1;

			if (is_identifier_character(cstr[0]))
			{
				for (; is_identifier_character(cstr[count]); count++);
			}

			return TypePattern{count, string_units_utf8(cstr, count)};
		}

		inline constexpr TypePattern find_type_pattern(const char * const cstr)
		{
			return
#if defined(__GNUG__)
				starts_with(string_units_utf8(cstr), "long long") ? TypePattern{ 9, string_units_utf8("long long int") } :
#elif defined(_MSC_VER)
				starts_with(string_units_utf8(cstr), "volatile const ") && is_identifier_character(*(cstr + 15)) ? TypePattern{ 15, string_units_utf8("const volatile ") } :
				starts_with(string_units_utf8(cstr), "volatile const") && !is_identifier_character(*(cstr + 14)) ? TypePattern{ 14, string_units_utf8("const volatile") } :
#endif
				starts_with(string_units_utf8(cstr), "const ") && is_identifier_character(*(cstr + 6)) ? TypePattern{ 6, string_units_utf8("const ") } :
				starts_with(string_units_utf8(cstr), "signed ") && is_identifier_character(*(cstr + 7)) ? TypePattern{ 7, string_units_utf8("signed ") } :
				starts_with(string_units_utf8(cstr), "unsigned ") && is_identifier_character(*(cstr + 9)) ? TypePattern{ 9, string_units_utf8("unsigned ") } :
				starts_with(string_units_utf8(cstr), "volatile ") && is_identifier_character(*(cstr + 9)) ? TypePattern{ 9, string_units_utf8("volatile ") } :
				starts_with(string_units_utf8(cstr), "long double") ? TypePattern{ 11, string_units_utf8("long double") } :
				starts_with(string_units_utf8(cstr), "long") && !is_identifier_character(*(cstr + 4)) ? TypePattern{ 4, string_units_utf8("long int") } :
				starts_with(string_units_utf8(cstr), "short") && !is_identifier_character(*(cstr + 5)) ? TypePattern{ 5, string_units_utf8("short int") } :
#if defined(__GNUG__)
				starts_with(string_units_utf8(cstr), "nullptr_t") && !is_identifier_character(*(cstr + 9)) ? TypePattern{ 9, string_units_utf8("std::nullptr_t") } :
				starts_with(string_units_utf8(cstr), "(anonymous namespace)") ? TypePattern{ 21, string_units_utf8("anonymous-namespace") } :
#elif defined(_MSC_VER)
				starts_with(string_units_utf8(cstr), "class ") ? TypePattern{ 6, string_units_utf8() } :
				starts_with(string_units_utf8(cstr), "struct ") ? TypePattern{ 7, string_units_utf8() } :
				starts_with(string_units_utf8(cstr), "__int64") && !is_identifier_character(*(cstr + 7)) ? TypePattern{ 7, string_units_utf8("long long int") } :
				starts_with(string_units_utf8(cstr), "`anonymous-namespace'") ? TypePattern{ 21, string_units_utf8("anonymous-namespace") } :
#endif
				starts_with(string_units_utf8(cstr), " ") ? TypePattern{ 1, string_units_utf8() } :
				next_word(cstr);
		}

		inline constexpr std::size_t length_of_type_name(string_units_utf8 str)
		{
			std::size_t length = 0;

			for (std::size_t i = 0; i < str.size();)
			{
				const TypePattern pattern = find_type_pattern(str.data() + i);

				length += pattern.with.size();
				i += pattern.replace;
			}

			return length;
		}

		// because c++14 does not have non-const constexpr subscript operator
		// on std::array
		template <std::size_t N>
		struct constexpr_array
		{
			char values[N];

			constexpr const char * data() const { return values; }
		};

		template <std::size_t N>
		constexpr constexpr_array<N> build_type_name(string_units_utf8 str)
		{
			constexpr_array<N> name{};
			std::ptrdiff_t length = 0;

			for (std::size_t i = 0; i < str.size();)
			{
				const TypePattern pattern = find_type_pattern(str.data() + i);

#if defined(_MSC_VER)
				// the microsoft compiler does not support constexpr
				// range for :facepalm:
				//
				// https://developercommunity.visualstudio.com/content/problem/227884/constexpr-foreach-doesnt-compile.html
				for (std::size_t j = 0; j < pattern.with.size(); j++)
#else
				for (std::ptrdiff_t j : ranges::index_sequence_for(pattern.with))
#endif
				{
					name.values[length + j] = pattern.with.data()[j];
				}

				length += pattern.with.size();
				i += pattern.replace;
			}

			return name;
		}
	}

	template <typename T>
	struct type_info
	{
		static constexpr string_units_utf8 type_signature = detail::get_type_signature<T>();
		static constexpr auto type_name_length = detail::length_of_type_name(type_signature);
		static constexpr detail::constexpr_array<type_name_length> type_name = detail::build_type_name<type_name_length>(type_signature);
	};

	template <typename T>
	constexpr string_units_utf8 type_info<T>::type_signature;
	template <typename T>
	constexpr detail::constexpr_array<type_info<T>::type_name_length> type_info<T>::type_name;

#if defined(_MSC_VER)
	// the windows compiler does something very weird here where the signature
	// of `std::nullptr_t` is "nullptr", while the signature of
	// `my_template<std::nullptr_t>` is "my_template<std::nullptr_t>", because
	// of that we cannot add this as a substitution above but instead hack it
	// here
	template <>
	struct type_info<std::nullptr_t>
	{
		static constexpr auto type_signature = detail::get_type_signature<std::nullptr_t>();
		static constexpr auto type_name_length = std::size_t(14);
		static constexpr auto type_name = detail::constexpr_array<14>{'s', 't', 'd', ':', ':', 'n', 'u', 'l', 'l', 'p', 't', 'r', '_', 't'};
	};
#endif

	using type_id_t = uint32_t;
	using type_name_t = string_units_utf8;
	using type_signature_t = string_units_utf8;

	/*
	 * \note The type id is plaform independant. Though keep in mind that aliases
	 * like `std::int64_t` is not the same type on all platforms!
	 */
	template <typename T>
	constexpr type_id_t type_id() { return crypto::crc32(type_info<T>::type_name.data(), type_info<T>::type_name_length); }

	/*
	 * \note The type name is platform independant. Though keep in mind that
	 * aliases like `std::int64_t` is not the same type on all platforms!
	 */
	template <typename T>
	constexpr type_name_t type_name() { return string_units_utf8(type_info<T>::type_name.data(), type_info<T>::type_name_length); }

	/*
	 * \note The type signature is platform dependant!
	 */
	template <typename T>
	constexpr type_signature_t type_signature() { return type_info<T>::type_signature; }
}
