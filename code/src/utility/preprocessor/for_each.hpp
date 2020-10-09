#pragma once

// #include "utility/preprocessor/common.hpp"
// #include "utility/preprocessor/count_arguments.hpp"

#define PP_FOR_EACH_1(macro, _1) macro(0, _1)
#define PP_FOR_EACH_2(macro, _1, _2) macro(0, _1) macro(1, _2)
#define PP_FOR_EACH_3(macro, _1, _2, _3) macro(0, _1) macro(1, _2) macro(2, _3)
#define PP_FOR_EACH_4(macro, _1, _2, _3, _4) macro(0, _1) macro(1, _2) macro(2, _3) macro(3, _4)
#define PP_FOR_EACH_5(macro, _1, _2, _3, _4, _5) macro(0, _1) macro(1, _2) macro(2, _3) macro(3, _4) macro(4, _5)
#define PP_FOR_EACH_6(macro, _1, _2, _3, _4, _5, _6) macro(0, _1) macro(1, _2) macro(2, _3) macro(3, _4) macro(4, _5) macro(5, _6)
#define PP_FOR_EACH_7(macro, _1, _2, _3, _4, _5, _6, _7) macro(0, _1) macro(1, _2) macro(2, _3) macro(3, _4) macro(4, _5) macro(5, _6) macro(6, _7)
#define PP_FOR_EACH_8(macro, _1, _2, _3, _4, _5, _6, _7, _8) macro(0, _1) macro(1, _2) macro(2, _3) macro(3, _4) macro(4, _5) macro(5, _6) macro(6, _7) macro(7, _8)
#define ____PP_FOR_EACH(n, macro, ...) PP_UNPACK(PP_FOR_EACH_##n(macro, __VA_ARGS__))
#define __PP_FOR_EACH(n, macro, ...) ____PP_FOR_EACH(n, macro, __VA_ARGS__)
#define PP_FOR_EACH(macro, ...) __PP_FOR_EACH(PP_COUNT_ARGUMENTS(__VA_ARGS__), macro, __VA_ARGS__)
