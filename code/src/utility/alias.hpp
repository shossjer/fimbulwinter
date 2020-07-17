#pragma once

// #include "utility/algorithm.hpp"
// #include "utility/preprocessor/for_each.hpp"
// #include "utility/type_traits.hpp"

// #include <utility>

#define _define_alias_declare_type(n, name) typename T##n,
#define _define_alias_declare_member(n, name) T##n name;

#define define_alias(name, ...) \
namespace detail \
{ \
	template <PP_FOR_EACH(_define_alias_declare_type, __VA_ARGS__) typename> \
	struct name \
	{ \
		PP_FOR_EACH(_define_alias_declare_member, __VA_ARGS__) \
	}; \
} \
auto name = [](auto && tuple) \
{ \
	return ext::apply([](auto && ...ps) \
	{ \
		return detail::name<decltype(ps) &&..., int>{std::forward<decltype(ps)>(ps)...}; \
	}, \
	std::forward<decltype(tuple)>(tuple)); \
}
