cmake_minimum_required(VERSION 3.1)

include(CMakeDependentOption)

project(dep NONE)

set(DEP_RELEASE_CONFIG Release CACHE STRING "Configuration type to use for dependencies.")
set(DEP_DEBUG_CONFIG "" CACHE STRING "Configuration type to use for dependencies, in debug. Leave empty to disable debug builds.")

if("${DEP_DEBUG_CONFIG}" STREQUAL "${DEP_RELEASE_CONFIG}")
	message(WARNING "DEP_DEBUG_CONFIG equals DEP_RELEASE_CONFIG, will ignore it")
	set(DEP_DEBUG_CONFIG "")
endif()

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
	get_property(_debug_configs GLOBAL PROPERTY DEBUG_CONFIGURATIONS)

	if(DEP_DEBUG_CONFIG AND "${CMAKE_BUILD_TYPE}" IN_LIST _debug_configs)
		set(CMAKE_BUILD_TYPE ${DEP_DEBUG_CONFIG})
	else()
		set(CMAKE_BUILD_TYPE ${DEP_RELEASE_CONFIG})
	endif()
endif()

option(BUILD_CATCH "Build catch" ON)
option(BUILD_FREETYPE "Build freetype" ON)
option(BUILD_JSON "Build nlohmann-json" ON)
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
	   NOT "${_cache_variable}" STREQUAL "CMAKE_INSTALL_PREFIX" AND # we add this in CmakeLists.txt.in
	   NOT "${_cache_variable}" STREQUAL "CMAKE_PREFIX_PATH") # we add this in CmakeLists.txt.in
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

	if(DEP_DEBUG_CONFIG)
		set(_configs ${DEP_RELEASE_CONFIG} ${DEP_DEBUG_CONFIG})
	else()
		set(_configs ${DEP_RELEASE_CONFIG})
	endif()

	foreach(_config IN ITEMS ${_configs})
		execute_process(
			COMMAND ${CMAKE_COMMAND} --build . --config ${_config}
			WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/generated"
			)
	endforeach()
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
