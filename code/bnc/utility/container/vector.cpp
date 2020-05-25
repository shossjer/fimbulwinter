#include "utility/container/vector.hpp"

#include <catch2/catch.hpp>

#include <vector>

TEST_CASE("vector emplace", "")
{
	BENCHMARK_ADVANCED("emplace std::vector<ld>")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<long double> values;
		values.reserve(meter.runs());
		meter.measure([&](int i){ values.emplace_back(static_cast<long double>(i)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<ld>")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<long double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(static_cast<long double>(i)); });
	};

	BENCHMARK_ADVANCED("emplace std::vector<i, d, c>")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<int> values0;
		std::vector<double> values1;
		std::vector<char> values2;
		values0.reserve(meter.runs());
		values1.reserve(meter.runs());
		values2.reserve(meter.runs());
		meter.measure([&](int i)
		              {
			              values0.emplace_back(static_cast<int>(i));
			              values1.emplace_back(static_cast<double>(i));
			              values2.emplace_back(static_cast<char>(i));
		              });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d, c>")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double, char> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(static_cast<int>(i), static_cast<double>(i), static_cast<char>(i)); });
	};
}
