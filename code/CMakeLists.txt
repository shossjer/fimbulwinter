option(RUN_TESTS "Run tests as part of the build process." ON)

set(test_compile_options
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
		-Wno-sign-conversion>
	# sign-conversion - Warn for implicit conversions that may change
	# the sign of an integer value, like assigning a signed integer
	# expression to an unsigned integer variable.
	$<$<CXX_COMPILER_ID:Clang>:
		-Wno-self-move>
	# self-move - explicitly moving variable of type A to itself
	$<$<CXX_COMPILER_ID:MSVC>:
		/wd4456>
	# C4456 - declaration of 'identifier' hides previous local
	# declaration
	)

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
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		)
	add_custom_target(
		${name}
		ALL
		DEPENDS ${_test_state}
		)
endfunction()

include (src/CMakeLists.txt)
include (tst/CMakeLists.txt)

add_library(utility "")
target_sources(utility PRIVATE ${SOURCES_UTILITY} ${HEADERS_UTILITY} ${${CMAKE_CXX_COMPILER_ID}_UTILITY})
target_include_directories(utility PUBLIC "src")
target_link_libraries(utility PRIVATE generated PUBLIC dep_json)
target_compile_options(utility PRIVATE ${private_compile_options})
target_compile_definitions(utility PRIVATE ${private_compile_definitions})

add_executable(utilitytest "")
target_sources(utilitytest PRIVATE ${FILES_UTILITY})
target_include_directories(utilitytest PRIVATE "tst")
target_link_libraries(utilitytest PRIVATE generated utility dep_catch)
target_compile_options(utilitytest PRIVATE ${private_compile_options} ${test_compile_options})
target_compile_definitions(utilitytest PRIVATE ${private_compile_definitions})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(utilitytest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	run_tests(runutilitytest utilitytest)
endif()

add_library(core "")
if(RUN_TESTS)
	add_dependencies(core runutilitytest)
endif()
target_sources(core PRIVATE ${SOURCES_CORE} ${HEADERS_CORE})
target_include_directories(core PUBLIC "src")
target_link_libraries(core PRIVATE generated utility PUBLIC dep_zlib dep_png thread_dep)
target_compile_options(core PRIVATE ${private_compile_options})
target_compile_definitions(core PRIVATE ${private_compile_definitions})

add_executable(coretest "")
target_sources(coretest PRIVATE ${FILES_CORE})
target_include_directories(coretest PRIVATE "tst")
target_link_libraries(coretest PRIVATE generated utility core dep_catch)
target_compile_options(coretest PRIVATE ${private_compile_options} ${test_compile_options})
target_compile_definitions(coretest PRIVATE ${private_compile_definitions})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(coretest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	run_tests(runcoretest coretest)
endif()

add_library(engine "")
if(RUN_TESTS)
	add_dependencies(engine runcoretest)
endif()
target_sources(engine PRIVATE ${SOURCES_ENGINE} ${HEADERS_ENGINE})
target_include_directories(engine PUBLIC "src")
target_link_libraries(engine PRIVATE generated utility core PUBLIC audio_dep graphics_dep input_dep text_dep window_dep)
target_compile_options(engine PRIVATE ${private_compile_options})
target_compile_definitions(engine PRIVATE ${private_compile_definitions})

add_executable(enginetest "")
target_sources(enginetest PRIVATE ${FILES_ENGINE})
target_include_directories(enginetest PRIVATE "tst")
target_link_libraries(enginetest PRIVATE generated utility core engine dep_catch)
target_compile_options(enginetest PRIVATE ${private_compile_options} ${test_compile_options})
target_compile_definitions(enginetest PRIVATE ${private_compile_definitions})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(enginetest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	run_tests(runenginetest enginetest)
endif()
