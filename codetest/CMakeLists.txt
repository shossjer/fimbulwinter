
option(ENABLE_TEST "Enable Catch tests" ON)

if(ENABLE_TEST)
	message(STATUS "Catch tests are enabled")

	include (src/CMakeLists.txt)

	add_executable(codetest "")
	target_sources(codetest PRIVATE ${CODE_TEST_SOURCE} ${CODE_TEST_HEADER})
	target_include_directories(codetest PRIVATE "src" "${GENERAL_INCLUDES}")
	target_link_libraries(codetest PRIVATE generated utility core engine gameplay ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})

	if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
		set_target_properties(codetest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
	endif()
else()
	message(STATUS "Catch tests are disabled")
endif(ENABLE_TEST)
