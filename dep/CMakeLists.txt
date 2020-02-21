cmake_minimum_required(VERSION 3.1)

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

set(_initial_cache "")
get_cmake_property(_cache_variables CACHE_VARIABLES)
foreach(_cache_variable IN ITEMS ${_cache_variables})
	if("${_cache_variable}" MATCHES "^CMAKE_" AND
	   NOT "${_cache_variable}" MATCHES "^CMAKE_CACHE" AND
	   NOT "${_cache_variable}" MATCHES "^CMAKE_PLATFORM_" AND
	   NOT "${_cache_variable}" MATCHES "^CMAKE_PROJECT_" AND
	   NOT "${_cache_variable}" MATCHES "^CMAKE_SIZEOF_" AND
	   NOT "${_cache_variable}" STREQUAL "CMAKE_HOME_DIRECTORY" AND
	   NOT "${_cache_variable}" STREQUAL "CMAKE_NUMBER_OF_MAKEFILES" AND
	   NOT "${_cache_variable}" STREQUAL "CMAKE_INSTALL_PREFIX" AND # we always add it last
	   NOT "${_cache_variable}" STREQUAL "CMAKE_PREFIX_PATH") # we always add it last
		get_property(_type CACHE ${_cache_variable} PROPERTY TYPE)
		set(_initial_cache "${_initial_cache}\nset(${_cache_variable} \"${${_cache_variable}}\" CACHE ${_type} \"Initial cache\")")
	endif()
endforeach()
unset(_type)
unset(_cache_variable)
unset(_cache_variables)
set(_initial_cache "${_initial_cache}\nset(CMAKE_INSTALL_PREFIX \"${PROJECT_SOURCE_DIR}\" CACHE PATH \"Initial cache\")")
set(_initial_cache "${_initial_cache}\nset(CMAKE_PREFIX_PATH \"${PROJECT_SOURCE_DIR}\" CACHE STRING \"Initial cache\")")

set(_initial_cache_file "${PROJECT_BINARY_DIR}/generated/initial_cache.cmake")
configure_file(
	"${PROJECT_SOURCE_DIR}/initial_cache.cmake.in"
	${_initial_cache_file}
	@ONLY
	)

unset(_initial_cache)

configure_file(
	"${PROJECT_SOURCE_DIR}/CMakeLists.txt.in"
	"${PROJECT_BINARY_DIR}/generated/CMakeLists.txt"
	@ONLY
	)

unset(_initial_cache_file)

execute_process(
	COMMAND ${CMAKE_COMMAND} -C initial_cache.cmake .
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
