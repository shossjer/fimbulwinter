cmake_minimum_required(VERSION 3.1)

include(CMakeDependentOption)

project(tools NONE)

set(TOOLS_RELEASE_CONFIG Release CACHE STRING "Configuration type to use for tools.")

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE ${TOOLS_RELEASE_CONFIG})
endif()

cmake_dependent_option(ENABLE_COMPILER_EXPLORER "Setup Compiler Explorer for use with this project." OFF "NOT ${CMAKE_VERSION} VERSION_LESS 3.14" OFF)
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
	   NOT "${_cache_variable}" STREQUAL "CMAKE_INSTALL_PREFIX" AND # (we add this in CmakeLists.txt.in)
	   NOT "${_cache_variable}" STREQUAL "CMAKE_PREFIX_PATH") # (we add this in CmakeLists.txt.in)
		get_property(_type CACHE ${_cache_variable} PROPERTY TYPE)
		set(_initial_cache "${_initial_cache}set(${_cache_variable} \"${${_cache_variable}}\" CACHE ${_type} \"Initial cache\")\n")
	endif()
endforeach()
unset(_type)
unset(_cache_variable)
unset(_cache_variables)

set(_initial_cache_file "${PROJECT_BINARY_DIR}/generated/initial_cache.cmake")
file(WRITE ${_initial_cache_file} ${_initial_cache})

unset(_initial_cache)

configure_file(
	"${PROJECT_SOURCE_DIR}/CMakeLists.txt.in"
	"${PROJECT_BINARY_DIR}/generated/CMakeLists.txt"
	@ONLY
	)

unset(_initial_cache_file)

if(DEFINED CMAKE_CONFIGURATION_TYPES)
	execute_process(
		COMMAND ${CMAKE_COMMAND} .
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
		)

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build . --config ${TOOLS_RELEASE_CONFIG}
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
		)
else()
	execute_process(
		COMMAND ${CMAKE_COMMAND} . -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
		)

	execute_process(
		COMMAND ${CMAKE_COMMAND} --build .
		WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
		)
endif()
