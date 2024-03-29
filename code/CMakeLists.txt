function(run_tests name executable)
	# inspired by the works of stack overflow user 1178599
	#
	# https://stackoverflow.com/a/53673873
	set(_test_state "${PROJECT_BINARY_DIR}/run_tests/${executable}.passed")
	add_custom_command(
		OUTPUT ${_test_state}
		COMMAND $<TARGET_FILE:${executable}>
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/run_tests"
		COMMAND ${CMAKE_COMMAND} -E touch ${_test_state}
		DEPENDS ${executable}
		WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
		)
	add_custom_target(
		${name}
		ALL
		DEPENDS ${_test_state}
		)
endfunction()

set(private_compile_options
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
		-Wall -Wextra -Wconversion -Werror -Wno-sign-conversion>
	# sign-conversion - Warn for implicit conversions that may change
	# the sign of an integer value, like assigning a signed integer
	# expression to an unsigned integer variable.
	$<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_LESS:CXX_COMPILER_VERSION,6.0>>:
		-Wno-missing-braces>
	# missing-braces - Warn if an aggregate or union initializer is not
	# fully bracketed.
	$<$<CXX_COMPILER_ID:MSVC>:
		/W4 /WX
		/constexpr:steps5000000
		/constexpr:depth700>
	)

set(private_compile_definitions
	$<$<CXX_COMPILER_ID:MSVC>:
		NOMINMAX UNICODE _UNICODE>
	)

include (bnc/CMakeLists.txt)
include (src/CMakeLists.txt)
include (tst/CMakeLists.txt)

add_library(utility "")
target_sources(utility PRIVATE ${SOURCES_UTILITY} ${HEADERS_UTILITY} ${${CMAKE_CXX_COMPILER_ID}_UTILITY})
target_include_directories(utility PUBLIC "src")
target_link_libraries(utility PRIVATE generated fiw_profiling fiolib fullib PUBLIC fiw_utility_features)
target_compile_options(utility PRIVATE ${private_compile_options})
target_compile_definitions(utility PRIVATE ${private_compile_definitions})

set(utility_dependency utility)

if(FIW_BUILD_TESTS)
	add_executable(utilitytest "")
	target_sources(utilitytest PRIVATE ${FILES_UTILITY})
	target_include_directories(utilitytest PRIVATE "tst")
	target_link_libraries(utilitytest PRIVATE generated utility fiw_test fiolib fullib)
	target_compile_options(utilitytest PRIVATE ${private_compile_options})
	target_compile_definitions(utilitytest PRIVATE ${private_compile_definitions})

	if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
		set_target_properties(utilitytest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}")
	endif()
endif()

if(FIW_TESTS_BUILDTIME)
	run_tests(runutilitytest utilitytest)
	set(utility_dependency runutilitytest)
endif()

if(BUILD_BENCHMARKS)
	add_executable(utilitybenchmark "")
	target_sources(utilitybenchmark PRIVATE ${BNC_UTILITY})
	target_include_directories(utilitybenchmark PRIVATE "bnc")
	target_link_libraries(utilitybenchmark PRIVATE generated utility fiw_benchmark fiolib fullib)
	target_compile_options(utilitybenchmark PRIVATE ${private_compile_options})
	target_compile_definitions(utilitybenchmark PRIVATE ${private_compile_definitions})

	if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
		set_target_properties(utilitybenchmark PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}")
	endif()
endif()

add_library(core "")
add_dependencies(core ${utility_dependency})
target_sources(core PRIVATE ${SOURCES_CORE} ${HEADERS_CORE} ${${CMAKE_CXX_COMPILER_ID}_CORE})
target_include_directories(core PUBLIC "src")
target_link_libraries(core PRIVATE generated utility fiw_profiling fiolib fullib PUBLIC thread_dep fiw_core_features)
target_compile_options(core PRIVATE ${private_compile_options})
target_compile_definitions(core PRIVATE ${private_compile_definitions})

set(core_dependency core)

if(FIW_BUILD_TESTS)
	add_executable(coretest "")
	target_sources(coretest PRIVATE ${FILES_CORE})
	target_include_directories(coretest PRIVATE "tst")
	target_link_libraries(coretest PRIVATE generated utility core fiw_test fiolib fullib)
	target_compile_options(coretest PRIVATE ${private_compile_options})
	target_compile_definitions(coretest PRIVATE ${private_compile_definitions})

	if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
		set_target_properties(coretest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}")
	endif()
endif()

if(FIW_TESTS_BUILDTIME)
	run_tests(runcoretest coretest)
	set(core_dependency runcoretest)
endif()

add_library(engine "")
add_dependencies(engine ${core_dependency})
target_sources(engine PRIVATE ${SOURCES_ENGINE} ${HEADERS_ENGINE})
target_include_directories(engine PUBLIC "src")
target_link_libraries(engine PRIVATE generated utility core fiw_profiling fiolib fullib PUBLIC audio_dep graphics_dep input_dep text_dep window_dep fiw_engine_features)
target_compile_options(engine PRIVATE ${private_compile_options})
target_compile_definitions(engine PRIVATE ${private_compile_definitions})

if(FIW_BUILD_TESTS)
	add_executable(enginetest "")
	target_sources(enginetest PRIVATE ${FILES_ENGINE})
	target_include_directories(enginetest PRIVATE "tst")
	target_link_libraries(enginetest PRIVATE generated utility core engine fiw_test fiolib fullib)
	target_compile_options(enginetest PRIVATE ${private_compile_options})
	target_compile_definitions(enginetest PRIVATE ${private_compile_definitions})

	if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
		set_target_properties(enginetest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}")
	endif()
endif()

if(FIW_TESTS_BUILDTIME)
	run_tests(runenginetest enginetest)
endif()
