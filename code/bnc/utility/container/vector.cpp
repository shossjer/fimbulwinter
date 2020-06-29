#include "utility/container/vector.hpp"

#include <catch2/catch.hpp>

#include <vector>

TEST_CASE("vector emplace", "")
{
	BENCHMARK_ADVANCED("emplace ld array[]")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<long double> values;
		values.resize(meter.runs());
		long double * ptr = values.data();
		meter.measure([&](int i){ *ptr++ = static_cast<long double>(i); });
	};

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

	BENCHMARK_ADVANCED("emplace heap_vector<ld> (no_reallocate)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<long double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_reallocate, static_cast<long double>(i)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<ld> (no_failure)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<long double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_failure, static_cast<long double>(i)); });
	};

	BENCHMARK_ADVANCED("emplace i, d array[]")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<int> values0;
		std::vector<double> values1;
		values0.resize(meter.runs());
		values1.resize(meter.runs());
		int * ptr0 = values0.data();
		double * ptr1 = values1.data();
		meter.measure([&](int i)
		              {
			              *ptr0++ = static_cast<int>(i * 3);
			              *ptr1++ = static_cast<double>(i * 5);
		              });
	};

	BENCHMARK_ADVANCED("emplace std::vector<i, d>")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<int> values0;
		std::vector<double> values1;
		values0.reserve(meter.runs());
		values1.reserve(meter.runs());
		meter.measure([&](int i)
		{
			values0.emplace_back(static_cast<int>(i * 3));
			values1.emplace_back(static_cast<double>(i * 5));
		});
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d>")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(static_cast<int>(i * 3), static_cast<double>(i * 5)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d> (no_reallocate)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_reallocate, static_cast<int>(i * 3), static_cast<double>(i * 5)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d> (no_failure)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_failure, static_cast<int>(i * 3), static_cast<double>(i * 5)); });
	};

	BENCHMARK_ADVANCED("emplace i, d, c array[]")(Catch::Benchmark::Chronometer meter)
	{
		std::vector<int> values0;
		std::vector<double> values1;
		std::vector<char> values2;
		values0.resize(meter.runs());
		values1.resize(meter.runs());
		values2.resize(meter.runs());
		int * ptr0 = values0.data();
		double * ptr1 = values1.data();
		char * ptr2 = values2.data();
		meter.measure([&](int i)
		{
			*ptr0++ = static_cast<int>(i * 3);
			*ptr1++ = static_cast<double>(i * 5);
			*ptr2++ = static_cast<char>(i * 2);
		});
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
			              values0.emplace_back(static_cast<int>(i * 3));
			              values1.emplace_back(static_cast<double>(i * 5));
			              values2.emplace_back(static_cast<char>(i * 2));
		              });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d, c>")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double, char> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(static_cast<int>(i * 3), static_cast<double>(i * 5), static_cast<char>(i * 2)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d, c> (no_reallocate)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double, char> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_reallocate, static_cast<int>(i * 3), static_cast<double>(i * 5), static_cast<char>(i * 2)); });
	};

	BENCHMARK_ADVANCED("emplace heap_vector<i, d, c> (no_failure)")(Catch::Benchmark::Chronometer meter)
	{
		utility::heap_vector<int, double, char> values;
		values.try_reserve(meter.runs());
		meter.measure([&](int i){ values.try_emplace_back(utility::no_failure, static_cast<int>(i * 3), static_cast<double>(i * 5), static_cast<char>(i * 2)); });
	};
}
