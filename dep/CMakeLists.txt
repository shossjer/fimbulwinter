cmake_minimum_required(VERSION 3.5)

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
option(BUILD_GLEXT "Build glext and other necessities" ON)
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

function(_try_package name target)
	set(_options SUPPORTS_DEBUG)
	set(_single_values)
	set(_multi_values DEPENDS ALL_FILES ONE_LIB)
	cmake_parse_arguments(_parsed "${_options}" "${_single_values}" "${_multi_values}" ${ARGN})

	cmake_parse_arguments(_parsed_ALL_FILES "" "" "IN" ${_parsed_ALL_FILES})
	set(_parsed_ALL_FILES ${_parsed_ALL_FILES_UNPARSED_ARGUMENTS})

	string(TOUPPER "${name}" _big_name)

	if(DEP_DEBUG_CONFIG AND _parsed_SUPPORTS_DEBUG)
		set(_configs ${DEP_RELEASE_CONFIG} ${DEP_DEBUG_CONFIG})
	else()
		set(_configs ${DEP_RELEASE_CONFIG})
	endif()

	set(_valid True)
	foreach(_config IN ITEMS ${_configs})
		string(TOUPPER "${_config}" _big_config)

		if(BUILD_${_big_name})
			set(_file_paths PATHS "${PROJECT_SOURCE_DIR}/${_config}/include" NO_DEFAULT_PATH)
			set(_lib_paths PATHS "${PROJECT_SOURCE_DIR}/${_config}/lib" NO_DEFAULT_PATH)
		else()
			set(_file_paths)
			set(_lib_paths)
		endif()

		if(_parsed_ALL_FILES_IN)
			set(_file_paths ${_file_paths} PATH_SUFFIXES ${_parsed_ALL_FILES_IN})
		endif()

		set(_found_all_files True)
		foreach(_file IN ITEMS ${_parsed_ALL_FILES})
			string(TOUPPER "${_file}" _big_file)
			string(MAKE_C_IDENTIFIER "${_big_file}" _c_file)

			find_file(FILE_${_c_file}_${_big_config} "${_file}" ${_file_paths})

			if(FILE_${_c_file}_${_big_config})
			 	message(STATUS "Looking for ${name} - ${FILE_${_c_file}_${_big_config}}")
			else()
			 	message(STATUS "Looking for ${name} - MISSING ${_file}")

				set(_found_all_files False)
			endif()
		endforeach()
		if(NOT _found_all_files)
			message(WARNING "${name} expected to find all files but some were MISSING!")
			set(_valid False)
		endif()

		if(_parsed_ONE_LIB)
			set(_found_one_lib_${_config})
			set(_colliding_libs False)
			foreach(_lib IN ITEMS ${_parsed_ONE_LIB})
				string(TOUPPER "${_lib}" _big_lib)
				string(MAKE_C_IDENTIFIER "${_big_lib}" _c_lib)

				find_library(LIBRARY_${_c_lib}_${_big_config} "${_lib}" ${_lib_paths})

				if(LIBRARY_${_c_lib}_${_big_config})
					if(_found_one_lib_${_config})
						message(STATUS "Looking for ${name} - COLLIDING ${LIBRARY_${_c_lib}_${_big_config}}")

						set(_colliding_libs True)
					else()
						message(STATUS "Looking for ${name} - ${LIBRARY_${_c_lib}_${_big_config}}")

						set(_found_one_lib_${_config} ${LIBRARY_${_c_lib}_${_big_config}})
					endif()
				else()
					message(STATUS "Looking for ${name} - MISSING ${_lib}")
				endif()
			endforeach()
			if(NOT _found_one_lib_${_config})
				message(WARNING "${name} expected to find one lib but all were MISSING!")
				set(_valid False)
			elseif(_colliding_libs)
				message(WARNING "${name} expected to find one lib but ${_found_one_lib_${_config}} is COLLIDING with other libraries!")
				set(_valid False)
			endif()
		endif()
	endforeach()

	foreach(_dep IN ITEMS ${_parsed_DEPENDS})
		if(NOT TARGET ${_dep})
			message(WARNING "${name} expected to depend on ${_dep} but it is not a target!")
			set(_valid False)
		endif()
	endforeach()

	if(_valid)
		foreach(_config IN ITEMS ${_configs})
			string(TOUPPER "${_config}" _big_config)

			add_library(${target}_${_config} INTERFACE)

			foreach(_file IN ITEMS ${_parsed_ALL_FILES})
				string(TOUPPER "${_file}" _big_file)
				string(MAKE_C_IDENTIFIER "${_big_file}" _c_file)

				string(REGEX REPLACE "/${_file}$" "" _directory "${FILE_${_c_file}_${_big_config}}")

				target_include_directories(${target}_${_config} INTERFACE "${_directory}")
			endforeach()

			target_link_libraries(${target}_${_config} INTERFACE "${_found_one_lib_${_config}}")
		endforeach()

		add_library(${target} INTERFACE)

		foreach(_dep IN ITEMS ${_parsed_DEPENDS})
			target_link_libraries(${target} INTERFACE ${_dep})
		endforeach()

		if(DEP_DEBUG_CONFIG AND _parsed_SUPPORTS_DEBUG)
			if(DEFINED CMAKE_CONFIGURATION_TYPES)
				target_link_libraries(${target} INTERFACE debug ${target}_${DEP_DEBUG_CONFIG} optimized ${target}_${DEP_RELEASE_CONFIG})
			else()
				target_link_libraries(${target} INTERFACE ${target}_${CMAKE_BUILD_TYPE})
			endif()
		else()
			target_link_libraries(${target} INTERFACE ${target}_${DEP_RELEASE_CONFIG})
		endif()
	endif()
endfunction()

_try_package(catch dep_catch ALL_FILES catch2/catch.hpp)
_try_package(glext dep_glext ALL_FILES GL/glext.h KHR/khrplatform.h)
_try_package(json dep_json ALL_FILES nlohmann/json.hpp)
_try_package(zlib dep_zlib ALL_FILES zlib.h ONE_LIB libz.a zlib)

_try_package(libpng dep_libpng DEPENDS dep_zlib ALL_FILES libpng16/png.h ONE_LIB libpng16.a libpng16_static)

_try_package(freetype dep_freetype DEPENDS dep_zlib dep_libpng ALL_FILES ft2build.h IN freetype2 ONE_LIB freetype)
