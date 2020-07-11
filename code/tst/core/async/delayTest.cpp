//#include "core/async/delay.hpp"
//
//#include <catch2/catch.hpp>
//
//#include <chrono>
//
//TEST_CASE("Delay function precision", "Verify delay function")
//{
//	const double SLEEP_MIN = 1.;
//	//
//	auto start = std::chrono::high_resolution_clock::now();
//	
//	core::async::delay(SLEEP_MIN*1000.);
//
//	auto end = std::chrono::high_resolution_clock::now();
//
//	std::chrono::duration<double> diff = end - start;
//
//	REQUIRE(diff.count() >= SLEEP_MIN);
//	REQUIRE(diff.count() <= SLEEP_MIN*1.01);	// within 1 %
//}
