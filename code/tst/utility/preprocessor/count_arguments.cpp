#include "utility/preprocessor/common.hpp"
#include "utility/preprocessor/count_arguments.hpp"

// todo no arguments should be counted as zero
static_assert(PP_COUNT_ARGUMENTS() != 0, "");

static_assert(PP_COUNT_ARGUMENTS(one) == 1, "");
static_assert(PP_COUNT_ARGUMENTS(, ) == 2, "");
static_assert(PP_COUNT_ARGUMENTS(three, , three) == 3, "");
static_assert(PP_COUNT_ARGUMENTS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) == 16, "");
