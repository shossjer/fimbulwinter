#include "utility/preprocessor/common.hpp"
#include "utility/preprocessor/count_arguments.hpp"
#include "utility/preprocessor/for_each.hpp"

namespace
{
#define macro_sum(index, arg) (index + arg),
	constexpr int result_sum[] = {PP_FOR_EACH(macro_sum, 8, 7, 6, 5, 4, 3, 2, 1)};
#undef macro_sum
	static_assert(sizeof result_sum / sizeof result_sum[0] == 8, "");
	static_assert(result_sum[0] == 8, "");
	static_assert(result_sum[1] == 8, "");
	static_assert(result_sum[2] == 8, "");
	static_assert(result_sum[3] == 8, "");
	static_assert(result_sum[4] == 8, "");
	static_assert(result_sum[5] == 8, "");
	static_assert(result_sum[6] == 8, "");
	static_assert(result_sum[7] == 8, "");
}
