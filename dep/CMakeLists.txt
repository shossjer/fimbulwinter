cmake_minimum_required(VERSION 3.0)

project(dep NONE)

if(DEFINED CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Detected multi configuration build: ${CMAKE_CONFIGURATION_TYPES}")

	if(DEFINED CMAKE_BUILD_TYPE)
		message(WARNING "Both CMAKE_CONFIGURATION_TYPES and CMAKE_BUILD_TYPE are set! Please unset CMAKE_BUILD_TYPE to avoid confusion.")
	endif()
elseif(DEFINED CMAKE_BUILD_TYPE)
	if(CMAKE_BUILD_TYPE STREQUAL "")
		message(STATUS "Detected single configuration build")
	else()
		message(STATUS "Detected single configuration build: ${CMAKE_BUILD_TYPE}")
	endif()
else()
	message(FATAL_ERROR "Neither CMAKE_CONFIGURATION_TYPES or CMAKE_BUILD_TYPE are set!")
endif()

option(BUILD_CATCH "Build catch" ON)
option(BUILD_FREETYPE "Build freetype" ON)
option(BUILD_NLOHMANN_JSON "Build nlohmann-json" ON)
option(BUILD_LIBPNG "Build libpng" ON)
option(BUILD_ZLIB "Build zlib" ON)

configure_file(
	"${PROJECT_SOURCE_DIR}/CMakeLists.txt.in"
	"${PROJECT_BINARY_DIR}/generated/CMakeLists.txt"
	@ONLY
	)

execute_process(
	COMMAND ${CMAKE_COMMAND} .
	WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
	)

if(CMAKE_CONFIGURATION_TYPES)
	foreach(_config IN ITEMS ${CMAKE_CONFIGURATION_TYPES})
		execute_process(
			COMMAND ${CMAKE_COMMAND} --build . --config ${_config}
			WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
			)
	endforeach()
else()
	execute_process(
		COMMAND ${CMAKE_COMMAND} --build .
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
		)
endif()
