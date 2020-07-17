#include "utility/member_info.hpp"

namespace
{
	struct E {};
	static_assert(utility::member_count<E>::value == 0, "");
#if (defined(_MSVC_LANG) && 201703L <=_MSVC_LANG) || 201703L <= __cplusplus
	static_assert(mpl::is_same<mpl::type_list<>, utility::member_types<E>>::value, "");
#endif

	struct S
	{
		int a;
		float b;
		char c;
		long double d;
	};
	static_assert(utility::member_count<S>::value == 4, "");
#if (defined(_MSVC_LANG) && 201703L <=_MSVC_LANG) || 201703L <= __cplusplus
	static_assert(mpl::is_same<mpl::type_list<int, float, char, long double>, utility::member_types<S>>::value, "");
#endif

	struct U
	{
		float a;
		float b;
		float c;
	};
	struct V
	{
		int a;
		U b;
		char c;
	};
	static_assert(utility::member_count<V>::value == 3, "");
#if (defined(_MSVC_LANG) && 201703L <=_MSVC_LANG) || 201703L <= __cplusplus
	static_assert(mpl::is_same<mpl::type_list<int, U, char>, utility::member_types<V>>::value, "");
#endif
}
