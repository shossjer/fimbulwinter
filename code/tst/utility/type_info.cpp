#include "utility/type_info.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct hidden_type;

	template <typename>
	struct template_type;

	using alias_type = int;

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif
	struct
	{
	} anonymous;
	using anonymous_type = decltype(anonymous);

	auto lambda = [](){};
	using lambda_type = decltype(lambda);

	auto function() { struct type {}; return type{}; }
	using in_function_type = decltype(function());
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
}

TEST_CASE("fundamental type signature", "[utility][type info]")
{
	SECTION("of void")
	{
		constexpr auto signature = utility::type_signature<void>();
		CHECK(signature == "void");
	}

	SECTION("of std::nullptr_t")
	{
		constexpr auto signature = utility::type_signature<std::nullptr_t>();
#if defined(__GNUG__)
		CHECK(signature == "nullptr_t");
#elif defined(_MSC_VER)
		CHECK(signature == "nullptr");
#endif
	}

	SECTION("of bool")
	{
		constexpr auto signature = utility::type_signature<bool>();
		CHECK(signature == "bool");
	}

	SECTION("of short int")
	{
		const char * const signature_str = "short";

		SECTION("alternative short")
		{
			constexpr auto signature = utility::type_signature<short>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative short int")
		{
			constexpr auto signature = utility::type_signature<short int>();
			CHECK(signature == signature_str);

			SECTION("permutation int short")
			{
				constexpr auto signature2 = utility::type_signature<int short>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed short")
		{
			constexpr auto signature = utility::type_signature<signed short>();
			CHECK(signature == signature_str);

			SECTION("permutation short signed")
			{
				constexpr auto signature2 = utility::type_signature<short signed>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed short int")
		{
			constexpr auto signature = utility::type_signature<signed short int>();
			CHECK(signature == signature_str);

			SECTION("permutation signed int short")
			{
				constexpr auto signature2 = utility::type_signature<signed int short>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation short signed int")
			{
				constexpr auto signature2 = utility::type_signature<short signed int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation short int signed")
			{
				constexpr auto signature2 = utility::type_signature<short int signed>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int signed short")
			{
				constexpr auto signature2 = utility::type_signature<int signed short>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int short signed")
			{
				constexpr auto signature2 = utility::type_signature<int short signed>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of unsigned short int")
	{
		const char * const signature_str = "unsigned short";

		SECTION("alternative unsigned short")
		{
			constexpr auto signature = utility::type_signature<unsigned short>();
			CHECK(signature == signature_str);

			SECTION("permutation short unsigned")
			{
				constexpr auto signature2 = utility::type_signature<short unsigned>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative unsigned short int")
		{
			constexpr auto signature = utility::type_signature<unsigned short int>();
			CHECK(signature == signature_str);

			SECTION("permutation unsigned int short")
			{
				constexpr auto signature2 = utility::type_signature<unsigned int short>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation short unsigned int")
			{
				constexpr auto signature2 = utility::type_signature<short unsigned int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation short int unsigned")
			{
				constexpr auto signature2 = utility::type_signature<short int unsigned>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int unsigned short")
			{
				constexpr auto signature2 = utility::type_signature<int unsigned short>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int short unsigned")
			{
				constexpr auto signature2 = utility::type_signature<int short unsigned>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of int")
	{
		const char * const signature_str = "int";

		SECTION("alternative int")
		{
			constexpr auto signature = utility::type_signature<int>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative signed")
		{
			constexpr auto signature = utility::type_signature<signed>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative signed int")
		{
			constexpr auto signature = utility::type_signature<signed int>();
			CHECK(signature == signature_str);

			SECTION("permutation int signed")
			{
				constexpr auto signature2 = utility::type_signature<int signed>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of unsigned int")
	{
		const char * const signature_str = "unsigned int";

		SECTION("alternative unsigned")
		{
			constexpr auto signature = utility::type_signature<unsigned>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative unsigned int")
		{
			constexpr auto signature = utility::type_signature<unsigned int>();
			CHECK(signature == signature_str);

			SECTION("permutation int unsigned")
			{
				constexpr auto signature2 = utility::type_signature<int unsigned>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of long int")
	{
		const char * const signature_str = "long";

		SECTION("alternative long")
		{
			constexpr auto signature = utility::type_signature<long>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative long int")
		{
			constexpr auto signature = utility::type_signature<long int>();
			CHECK(signature == signature_str);

			SECTION("permutation int long")
			{
				constexpr auto signature2 = utility::type_signature<int long>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed long")
		{
			constexpr auto signature = utility::type_signature<signed long>();
			CHECK(signature == signature_str);

			SECTION("permutation long signed")
			{
				constexpr auto signature2 = utility::type_signature<long signed>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed long int")
		{
			constexpr auto signature = utility::type_signature<signed long int>();
			CHECK(signature == signature_str);

			SECTION("permutation signed int long")
			{
				constexpr auto signature2 = utility::type_signature<signed int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long signed int")
			{
				constexpr auto signature2 = utility::type_signature<long signed int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int signed")
			{
				constexpr auto signature2 = utility::type_signature<long int signed>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int signed long")
			{
				constexpr auto signature2 = utility::type_signature<int signed long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long signed")
			{
				constexpr auto signature2 = utility::type_signature<int long signed>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of unsigned long int")
	{
		const char * const signature_str = "unsigned long";

		SECTION("alternative unsigned long")
		{
			constexpr auto signature = utility::type_signature<unsigned long>();
			CHECK(signature == signature_str);

			SECTION("permutation long unsigned")
			{
				constexpr auto signature2 = utility::type_signature<long unsigned>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative unsigned long int")
		{
			constexpr auto signature = utility::type_signature<unsigned long int>();
			CHECK(signature == signature_str);

			SECTION("permutation unsigned int long")
			{
				constexpr auto signature2 = utility::type_signature<unsigned int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long unsigned int")
			{
				constexpr auto signature2 = utility::type_signature<long unsigned int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int unsigned")
			{
				constexpr auto signature2 = utility::type_signature<long int unsigned>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int unsigned long")
			{
				constexpr auto signature2 = utility::type_signature<int unsigned long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long unsigned")
			{
				constexpr auto signature2 = utility::type_signature<int long unsigned>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of long long int")
	{
#if defined(__GNUG__)
		const char * const signature_str = "long long";
#elif defined(_MSC_VER)
		const char * const signature_str = "__int64";
#endif

		SECTION("alternative long long")
		{
			constexpr auto signature = utility::type_signature<long long>();
			CHECK(signature == signature_str);
		}

		SECTION("alternative long long int")
		{
			constexpr auto signature = utility::type_signature<long long int>();
			CHECK(signature == signature_str);

			SECTION("permutation long int long")
			{
				constexpr auto signature2 = utility::type_signature<long int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long long")
			{
				constexpr auto signature2 = utility::type_signature<int long long>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed long long")
		{
			constexpr auto signature = utility::type_signature<signed long long>();
			CHECK(signature == signature_str);

			SECTION("permutation long signed long")
			{
				constexpr auto signature2 = utility::type_signature<long signed long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long signed")
			{
				constexpr auto signature2 = utility::type_signature<long long signed>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative signed long long int")
		{
			constexpr auto signature = utility::type_signature<signed long long int>();
			CHECK(signature == signature_str);

			SECTION("permutation signed long int long")
			{
				constexpr auto signature2 = utility::type_signature<signed long int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation signed int long long")
			{
				constexpr auto signature2 = utility::type_signature<signed int long long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long signed long int")
			{
				constexpr auto signature2 = utility::type_signature<long signed long int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long signed int long")
			{
				constexpr auto signature2 = utility::type_signature<long signed int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long signed int")
			{
				constexpr auto signature2 = utility::type_signature<long long signed int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long int signed")
			{
				constexpr auto signature2 = utility::type_signature<long long int signed>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int signed long")
			{
				constexpr auto signature2 = utility::type_signature<long int signed long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int long signed")
			{
				constexpr auto signature2 = utility::type_signature<long int long signed>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int signed long long")
			{
				constexpr auto signature2 = utility::type_signature<int signed long long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long signed long")
			{
				constexpr auto signature2 = utility::type_signature<int long signed long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long long signed")
			{
				constexpr auto signature2 = utility::type_signature<int long long signed>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of unsigned long long int")
	{
#if defined(__GNUG__)
		const char * const signature_str = "unsigned long long";
#elif defined(_MSC_VER)
		const char * const signature_str = "unsigned __int64";
#endif

		SECTION("alternative unsigned long long")
		{
			constexpr auto signature = utility::type_signature<unsigned long long>();
			CHECK(signature == signature_str);

			SECTION("permutation long unsigned long")
			{
				constexpr auto signature2 = utility::type_signature<long unsigned long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long unsigned")
			{
				constexpr auto signature2 = utility::type_signature<long long unsigned>();
				CHECK(signature2 == signature_str);
			}
		}

		SECTION("alternative unsigned long long int")
		{
			constexpr auto signature = utility::type_signature<unsigned long long int>();
			CHECK(signature == signature_str);

			SECTION("permutation unsigned long int long")
			{
				constexpr auto signature2 = utility::type_signature<unsigned long int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation unsigned int long long")
			{
				constexpr auto signature2 = utility::type_signature<unsigned int long long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long unsigned long int")
			{
				constexpr auto signature2 = utility::type_signature<long unsigned long int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long unsigned int long")
			{
				constexpr auto signature2 = utility::type_signature<long unsigned int long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long unsigned int")
			{
				constexpr auto signature2 = utility::type_signature<long long unsigned int>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long long int unsigned")
			{
				constexpr auto signature2 = utility::type_signature<long long int unsigned>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int unsigned long")
			{
				constexpr auto signature2 = utility::type_signature<long int unsigned long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation long int long unsigned")
			{
				constexpr auto signature2 = utility::type_signature<long int long unsigned>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int unsigned long long")
			{
				constexpr auto signature2 = utility::type_signature<int unsigned long long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long unsigned long")
			{
				constexpr auto signature2 = utility::type_signature<int long unsigned long>();
				CHECK(signature2 == signature_str);
			}

			SECTION("permutation int long long unsigned")
			{
				constexpr auto signature2 = utility::type_signature<int long long unsigned>();
				CHECK(signature2 == signature_str);
			}
		}
	}

	SECTION("of signed char")
	{
		constexpr auto signature = utility::type_signature<signed char>();
		CHECK(signature == "signed char");
	}

	SECTION("of unsigned char")
	{
		constexpr auto signature = utility::type_signature<unsigned char>();
		CHECK(signature == "unsigned char");
	}

	SECTION("of char")
	{
		constexpr auto signature = utility::type_signature<char>();
		CHECK(signature == "char");
	}

	SECTION("of wchar_t")
	{
		constexpr auto signature = utility::type_signature<wchar_t>();
		CHECK(signature == "wchar_t");
	}

	SECTION("of char16_t")
	{
		constexpr auto signature = utility::type_signature<char16_t>();
		CHECK(signature == "char16_t");
	}

	SECTION("of char32_t")
	{
		constexpr auto signature = utility::type_signature<char32_t>();
		CHECK(signature == "char32_t");
	}

	SECTION("of float")
	{
		constexpr auto signature = utility::type_signature<float>();
		CHECK(signature == "float");
	}

	SECTION("of double")
	{
		constexpr auto signature = utility::type_signature<double>();
		CHECK(signature == "double");
	}

	SECTION("of long double")
	{
		const char * const signature_str = "long double";

		constexpr auto signature = utility::type_signature<long double>();
		CHECK(signature == signature_str);

		SECTION("permutation double long")
		{
			constexpr auto signature2 = utility::type_signature<double long>();
			CHECK(signature2 == signature_str);
		}
	}
}

TEST_CASE("fundamental type name", "[utility][type info]")
{
	SECTION("of void")
	{
		constexpr auto name = utility::type_name<void>();
		CHECK(name == "void");
	}

	SECTION("of std::nullptr_t")
	{
		constexpr auto name = utility::type_name<std::nullptr_t>();
		CHECK(name == "std::nullptr_t");
	}

	SECTION("of bool")
	{
		constexpr auto name = utility::type_name<bool>();
		CHECK(name == "bool");
	}

	SECTION("of short int")
	{
		constexpr auto name = utility::type_name<short int>();
		CHECK(name == "short int");
	}

	SECTION("of unsigned short int")
	{
		constexpr auto name = utility::type_name<unsigned short int>();
		CHECK(name == "unsigned short int");
	}

	SECTION("of int")
	{
		constexpr auto name = utility::type_name<int>();
		CHECK(name == "int");
	}

	SECTION("of unsigned int")
	{
		constexpr auto name = utility::type_name<unsigned int>();
		CHECK(name == "unsigned int");
	}

	SECTION("of long int")
	{
		constexpr auto name = utility::type_name<long int>();
		CHECK(name == "long int");
	}

	SECTION("of unsigned long int")
	{
		constexpr auto name = utility::type_name<unsigned long int>();
		CHECK(name == "unsigned long int");
	}

	SECTION("of long long int")
	{
		constexpr auto name = utility::type_name<long long int>();
		CHECK(name == "long long int");
	}

	SECTION("of unsigned long long int")
	{
		constexpr auto name = utility::type_name<unsigned long long int>();
		CHECK(name == "unsigned long long int");
	}

	SECTION("of signed char")
	{
		constexpr auto name = utility::type_name<signed char>();
		CHECK(name == "signed char");
	}

	SECTION("of unsigned char")
	{
		constexpr auto name = utility::type_name<unsigned char>();
		CHECK(name == "unsigned char");
	}

	SECTION("of char")
	{
		constexpr auto name = utility::type_name<char>();
		CHECK(name == "char");
	}

	SECTION("of wchar_t")
	{
		constexpr auto name = utility::type_name<wchar_t>();
		CHECK(name == "wchar_t");
	}

	SECTION("of char16_t")
	{
		constexpr auto name = utility::type_name<char16_t>();
		CHECK(name == "char16_t");
	}

	SECTION("of char32_t")
	{
		constexpr auto name = utility::type_name<char32_t>();
		CHECK(name == "char32_t");
	}

	SECTION("of float")
	{
		constexpr auto name = utility::type_name<float>();
		CHECK(name == "float");
	}

	SECTION("of double")
	{
		constexpr auto name = utility::type_name<double>();
		CHECK(name == "double");
	}

	SECTION("of long double")
	{
		constexpr auto name = utility::type_name<long double>();
		CHECK(name == "long double");
	}
}

TEST_CASE("fundamental type id", "[utility][type info]")
{
	SECTION("of void")
	{
		constexpr auto id = utility::type_id<void>();
		CHECK(id == utility::crypto::crc32("void"));
	}

	SECTION("of std::nullptr_t")
	{
		constexpr auto id = utility::type_id<std::nullptr_t>();
		CHECK(id == utility::crypto::crc32("std::nullptr_t"));
	}

	SECTION("of bool")
	{
		constexpr auto id = utility::type_id<bool>();
		CHECK(id == utility::crypto::crc32("bool"));
	}

	SECTION("of short int")
	{
		constexpr auto id = utility::type_id<short int>();
		CHECK(id == utility::crypto::crc32("short int"));
	}

	SECTION("of unsigned short int")
	{
		constexpr auto id = utility::type_id<unsigned short int>();
		CHECK(id == utility::crypto::crc32("unsigned short int"));
	}

	SECTION("of int")
	{
		constexpr auto id = utility::type_id<int>();
		CHECK(id == utility::crypto::crc32("int"));
	}

	SECTION("of unsigned int")
	{
		constexpr auto id = utility::type_id<unsigned int>();
		CHECK(id == utility::crypto::crc32("unsigned int"));
	}

	SECTION("of long int")
	{
		constexpr auto id = utility::type_id<long int>();
		CHECK(id == utility::crypto::crc32("long int"));
	}

	SECTION("of unsigned long int")
	{
		constexpr auto id = utility::type_id<unsigned long int>();
		CHECK(id == utility::crypto::crc32("unsigned long int"));
	}

	SECTION("of long long int")
	{
		constexpr auto id = utility::type_id<long long int>();
		CHECK(id == utility::crypto::crc32("long long int"));
	}

	SECTION("of unsigned long long int")
	{
		constexpr auto id = utility::type_id<unsigned long long int>();
		CHECK(id == utility::crypto::crc32("unsigned long long int"));
	}

	SECTION("of signed char")
	{
		constexpr auto id = utility::type_id<signed char>();
		CHECK(id == utility::crypto::crc32("signed char"));
	}

	SECTION("of unsigned char")
	{
		constexpr auto id = utility::type_id<unsigned char>();
		CHECK(id == utility::crypto::crc32("unsigned char"));
	}

	SECTION("of char")
	{
		constexpr auto id = utility::type_id<char>();
		CHECK(id == utility::crypto::crc32("char"));
	}

	SECTION("of wchar_t")
	{
		constexpr auto id = utility::type_id<wchar_t>();
		CHECK(id == utility::crypto::crc32("wchar_t"));
	}

	SECTION("of char16_t")
	{
		constexpr auto id = utility::type_id<char16_t>();
		CHECK(id == utility::crypto::crc32("char16_t"));
	}

	SECTION("of char32_t")
	{
		constexpr auto id = utility::type_id<char32_t>();
		CHECK(id == utility::crypto::crc32("char32_t"));
	}

	SECTION("of float")
	{
		constexpr auto id = utility::type_id<float>();
		CHECK(id == utility::crypto::crc32("float"));
	}

	SECTION("of double")
	{
		constexpr auto id = utility::type_id<double>();
		CHECK(id == utility::crypto::crc32("double"));
	}

	SECTION("of long double")
	{
		constexpr auto id = utility::type_id<long double>();
		CHECK(id == utility::crypto::crc32("long double"));
	}
}

TEST_CASE("const/volatile type signature", "[utility][type info]")
{
	SECTION("of const int")
	{
		const char * const signature_str = "const int";

		constexpr auto signature = utility::type_signature<const int>();
		CHECK(signature == signature_str);

		SECTION("permutation int const")
		{
			constexpr auto signature2 = utility::type_signature<int const>();
			CHECK(signature2 == signature_str);
		}
	}

	SECTION("of volatile int")
	{
		const char * const signature_str = "volatile int";

		constexpr auto signature = utility::type_signature<volatile int>();
		CHECK(signature == signature_str);

		SECTION("permutation int volatile")
		{
			constexpr auto signature2 = utility::type_signature<int volatile>();
			CHECK(signature2 == signature_str);
		}
	}

	SECTION("of const volatile int")
	{
#if defined(__GNUG__)
		const char * const signature_str = "const volatile int";
#elif defined(_MSC_VER)
		const char * const signature_str = "volatile const int";
#endif

		constexpr auto signature = utility::type_signature<const volatile int>();
		CHECK(signature == signature_str);

		SECTION("permutation const int volatile")
		{
			constexpr auto signature2 = utility::type_signature<const int volatile>();
			CHECK(signature2 == signature_str);
		}

		SECTION("permutation volatile const int")
		{
			constexpr auto signature2 = utility::type_signature<volatile const int>();
			CHECK(signature2 == signature_str);
		}

		SECTION("permutation volatile int const")
		{
			constexpr auto signature2 = utility::type_signature<volatile int const>();
			CHECK(signature2 == signature_str);
		}
	}

	SECTION("of const int * const * const")
	{
#if defined(__GNUG__)
		const char * const signature_str = "const int *const *const";
#elif defined(_MSC_VER)
		const char * const signature_str = "const int*const *const ";
#endif

		constexpr auto signature = utility::type_signature<const int * const * const>();
		CHECK(signature == signature_str);

		SECTION("permutation int const * const * const")
		{
			constexpr auto signature2 = utility::type_signature<int const * const * const>();
			CHECK(signature2 == signature_str);
		}
	}

	SECTION("of volatile int * volatile * volatile")
	{
#if defined(__GNUG__)
		const char * const signature_str = "volatile int *volatile *volatile";
#elif defined(_MSC_VER)
		const char * const signature_str = "volatile int*volatile *volatile ";
#endif

		constexpr auto signature = utility::type_signature<volatile int * volatile * volatile>();
		CHECK(signature == signature_str);

		SECTION("permutation int volatile * volatile * volatile")
		{
			constexpr auto signature2 = utility::type_signature<int volatile * volatile * volatile>();
			CHECK(signature2 == signature_str);
		}
	}

	SECTION("of int * const volatile * const volatile")
	{
#if defined(__GNUG__)
		const char * const signature_str = "int *const volatile *const volatile";
#elif defined(_MSC_VER)
		const char * const signature_str = "int*volatile const *volatile const ";
#endif

		constexpr auto signature = utility::type_signature<int * const volatile * const volatile>();
		CHECK(signature == signature_str);

		SECTION("permutation int * const volatile * volatile const")
		{
			constexpr auto signature2 = utility::type_signature<int * const volatile * volatile const>();
			CHECK(signature2 == signature_str);
		}

		SECTION("permutation int * volatile const * const volatile")
		{
			constexpr auto signature2 = utility::type_signature<int * volatile const * const volatile>();
			CHECK(signature2 == signature_str);
		}

		SECTION("permutation int * volatile const * volatile const")
		{
			constexpr auto signature2 = utility::type_signature<int * volatile const * volatile const>();
			CHECK(signature2 == signature_str);
		}
	}
}

TEST_CASE("const/volatile type name", "[utility][type info]")
{
	SECTION("of const int")
	{
		const char * const name_str = "const int";

		constexpr auto name = utility::type_name<const int>();
		CHECK(name == name_str);

		SECTION("permutation int const")
		{
			constexpr auto name2 = utility::type_name<int const>();
			CHECK(name2 == name_str);
		}
	}

	SECTION("of volatile int")
	{
		const char * const name_str = "volatile int";

		constexpr auto name = utility::type_name<volatile int>();
		CHECK(name == name_str);

		SECTION("permutation int volatile")
		{
			constexpr auto name2 = utility::type_name<int volatile>();
			CHECK(name2 == name_str);
		}
	}

	SECTION("of const volatile int")
	{
		const char * const name_str = "const volatile int";

		constexpr auto name = utility::type_name<const volatile int>();
		CHECK(name == name_str);

		SECTION("permutation const int volatile")
		{
			constexpr auto name2 = utility::type_name<const int volatile>();
			CHECK(name2 == name_str);
		}

		SECTION("permutation volatile const int")
		{
			constexpr auto name2 = utility::type_name<volatile const int>();
			CHECK(name2 == name_str);
		}

		SECTION("permutation volatile int const")
		{
			constexpr auto name2 = utility::type_name<volatile int const>();
			CHECK(name2 == name_str);
		}
	}

	SECTION("of const int * const * const")
	{
		const char * const name_str = "const int*const*const";

		constexpr auto name = utility::type_name<const int * const * const>();
		CHECK(name == name_str);

		SECTION("permutation int const * const * const")
		{
			constexpr auto name2 = utility::type_name<int const * const * const>();
			CHECK(name2 == name_str);
		}
	}

	SECTION("of volatile int * volatile * volatile")
	{
		constexpr auto name = utility::type_name<volatile int * volatile * volatile>();
		CHECK(name == "volatile int*volatile*volatile");
	}

	SECTION("of int * const volatile * const volatile")
	{
		constexpr auto name = utility::type_name<int * const volatile * const volatile>();
		CHECK(name == "int*const volatile*const volatile");
	}
}

TEST_CASE("const/volatile type id", "[utility][type info]")
{
	SECTION("of const int")
	{
		constexpr auto id = utility::type_id<const int>();
		CHECK(id == utility::crypto::crc32("const int"));
	}

	SECTION("of volatile int")
	{
		constexpr auto id = utility::type_id<volatile int>();
		CHECK(id == utility::crypto::crc32("volatile int"));
	}

	SECTION("of const volatile int")
	{
		constexpr auto id = utility::type_id<const volatile int>();
		CHECK(id == utility::crypto::crc32("const volatile int"));
	}

	SECTION("of const int * const * const")
	{
		constexpr auto id = utility::type_id<const int * const * const>();
		CHECK(id == utility::crypto::crc32("const int*const*const"));
	}

	SECTION("of volatile int * volatile * volatile")
	{
		constexpr auto id = utility::type_id<volatile int * volatile * volatile>();
		CHECK(id == utility::crypto::crc32("volatile int*volatile*volatile"));
	}

	SECTION("of int * const volatile * const volatile")
	{
		constexpr auto id = utility::type_id<int * const volatile * const volatile>();
		CHECK(id == utility::crypto::crc32("int*const volatile*const volatile"));
	}
}

TEST_CASE("type signature", "[utility][type info]")
{
	SECTION("of int **")
	{
		constexpr auto signature = utility::type_signature<int **>();
#if defined(__GNUG__)
		CHECK(signature == "int **");
#elif defined(_MSC_VER)
		CHECK(signature == "int**");
#endif
	}

	SECTION("of float(double, double)")
	{
		constexpr auto signature = utility::type_signature<float(double, double)>();
#if defined(__GNUG__)
		CHECK(signature == "float (double, double)");
#elif defined(_MSC_VER)
		CHECK(signature == "float(double,double)");
#endif
	}

	SECTION("of short[10]")
	{
		constexpr auto signature = utility::type_signature<short[10]>();
#if defined(__GNUG__)
		CHECK(signature == "short [10]");
#elif defined(_MSC_VER)
		CHECK(signature == "short[10]");
#endif
	}

	SECTION("of std::vector<int>")
	{
		constexpr auto signature = utility::type_signature<std::vector<int>>();
#if defined(__GNUG__)
		CHECK(signature == "std::vector<int, std::allocator<int> >");
#elif defined(_MSC_VER)
		CHECK(signature == "class std::vector<int,class std::allocator<int> >");
#endif
	}

	SECTION("of hidden_type")
	{
		constexpr auto signature = utility::type_signature<hidden_type>();
#if defined(__GNUG__)
		CHECK(signature == "(anonymous namespace)::hidden_type");
#elif defined(_MSC_VER)
		CHECK(signature == "struct `anonymous-namespace'::hidden_type");
#endif
	}

	SECTION("of template_type<std::nullptr_t>")
	{
		constexpr auto signature = utility::type_signature<template_type<std::nullptr_t>>();
#if defined(__GNUG__)
		CHECK(signature == "(anonymous namespace)::template_type<nullptr_t>");
#elif defined(_MSC_VER)
		CHECK(signature == "struct `anonymous-namespace'::template_type<std::nullptr_t>");
#endif
	}

	SECTION("of alias_type")
	{
		constexpr auto signature = utility::type_signature<alias_type>();
		CHECK(signature == "int");
	}

	SECTION("of anonymous_type")
	{
		constexpr auto signature = utility::type_signature<anonymous_type>();
		// we do not promise any name in particular for an anonymous type, but
		// everything up to the point of its name must be correct
#if defined(__GNUG__)
		CHECK(compare(signature.begin() + 0, signature.begin() + 23, "(anonymous namespace)::") == 0);
#elif defined(_MSC_VER)
		CHECK(compare(signature.begin() + 0, signature.begin() + 23, "`anonymous-namespace'::") == 0);
#endif
	}

	SECTION("of lambda_type")
	{
		constexpr auto signature = utility::type_signature<lambda_type>();
		// we do not promise any name in particular for a lambda type, but
		// everything up to the point of its name must be correct
#if defined(__GNUG__)
		CHECK(compare(signature.begin() + 0, signature.begin() + 23, "(anonymous namespace)::") == 0);
#elif defined(_MSC_VER)
		CHECK(compare(signature.begin() + 0, signature.begin() + 29, "class `anonymous-namespace'::") == 0);
#endif
	}

	SECTION("of type in function")
	{
		constexpr auto signature = utility::type_signature<in_function_type>();
		// we do not promise any name in particular for a type in a function,
		// simply because the compilers do very differently for this case
#if defined(__GNUG__)
		CHECK(signature == "type");
#elif defined(_MSC_VER)
		CHECK(signature == "struct `anonymous-namespace'::function::type");
#endif
	}
}

TEST_CASE("type name", "[utility][type info]")
{
	SECTION("of int **")
	{
		constexpr auto name = utility::type_name<int **>();
		CHECK(name == "int**");
	}

	SECTION("of float(double, double)")
	{
		constexpr auto name = utility::type_name<float(double, double)>();
		CHECK(name == "float(double,double)");
	}

	SECTION("of short[10]")
	{
		constexpr auto name = utility::type_name<short[10]>();
		CHECK(name == "short int[10]");
	}

	SECTION("of std::vector<int>")
	{
		constexpr auto name = utility::type_name<std::vector<int>>();
		CHECK(name == "std::vector<int,std::allocator<int>>");
	}

	SECTION("of hidden_type")
	{
		constexpr auto name = utility::type_name<hidden_type>();
		CHECK(name == "anonymous-namespace::hidden_type");
	}

	SECTION("of template_type<std::nullptr_t>")
	{
		constexpr auto name = utility::type_name<template_type<std::nullptr_t>>();
		CHECK(name == "anonymous-namespace::template_type<std::nullptr_t>");
	}

	SECTION("of alias_type")
	{
		constexpr auto name = utility::type_name<alias_type>();
		CHECK(name == "int");
	}

	SECTION("of anonymous_type")
	{
		constexpr auto name = utility::type_name<anonymous_type>();
		// we do not promise any name in particular for an anonymous type, but
		// everything up to the point of its name must be correct
		CHECK(compare(name.begin() + 0, name.begin() + 21, "anonymous-namespace::") == 0);
	}

	SECTION("of lambda_type")
	{
		constexpr auto name = utility::type_name<lambda_type>();
		// we do not promise any name in particular for a lambda type, but
		// everything up to the point of its name must be correct
		CHECK(compare(name.begin() + 0, name.begin() + 21, "anonymous-namespace::") == 0);
	}

	SECTION("of type in function")
	{
		constexpr auto name = utility::type_name<in_function_type>();
		// we do not promise any name in particular for a type in a function,
		// which means its name can be anything :shrug:
		static_cast<void>(name);
	}
}

TEST_CASE("type id", "[utility][type info]")
{
	SECTION("of int **")
	{
		constexpr auto id = utility::type_id<int **>();
		CHECK(id == utility::crypto::crc32("int**"));
	}

	SECTION("of float(double)")
	{
		constexpr auto id = utility::type_id<float(double, double)>();
		CHECK(id == utility::crypto::crc32("float(double,double)"));
	}

	SECTION("of short[10]")
	{
		constexpr auto id = utility::type_id<short[10]>();
		CHECK(id == utility::crypto::crc32("short int[10]"));
	}

	SECTION("of std::vector<int>")
	{
		constexpr auto id = utility::type_id<std::vector<int>>();
		CHECK(id == utility::crypto::crc32("std::vector<int,std::allocator<int>>"));
	}

	SECTION("of hidden_type")
	{
		constexpr auto id = utility::type_id<hidden_type>();
		CHECK(id == utility::crypto::crc32("anonymous-namespace::hidden_type"));
	}

	SECTION("of template_type<std::nullptr_t>")
	{
		constexpr auto id = utility::type_id<template_type<std::nullptr_t>>();
		CHECK(id == utility::crypto::crc32("anonymous-namespace::template_type<std::nullptr_t>"));
	}

	SECTION("of alias_type")
	{
		constexpr auto id = utility::type_id<alias_type>();
		CHECK(id == utility::crypto::crc32("int"));
	}

	SECTION("of anonymous_type")
	{
		constexpr auto id = utility::type_id<anonymous_type>();
		// we do not promise any name in particular for an anonymous type,
		// which means its id can be anything :shrug:
		static_cast<void>(id);
	}

	SECTION("of lambda_type")
	{
		constexpr auto id = utility::type_id<lambda_type>();
		// we do not promise any name in particular for a lambda type, which
		// means its id can be anything :shrug:
		static_cast<void>(id);
	}

	SECTION("of type in function")
	{
		constexpr auto id = utility::type_id<in_function_type>();
		// we do not promise any name in particular for a type in a function,
		// which means its name can be anything :shrug:
		static_cast<void>(id);
	}
}
