cmake_minimum_required(VERSION 3.1)

include(CMakeDependentOption)

macro(_set_configs outvar name)
	string(TOUPPER "${name}" _big_name)

	if(ENABLE_${_big_name})
		set(${outvar} ${TOOLS_RELEASE_CONFIG})
	else()
		set(${outvar})
	endif()
endmacro()

project(tools NONE)

set(TOOLS_RELEASE_CONFIG Release CACHE STRING "Configuration type to use for tools.")

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE ${TOOLS_RELEASE_CONFIG})
endif()

cmake_dependent_option(ENABLE_COMPILER_EXPLORER "Setup Compiler Explorer for use with this project." OFF "NOT ${CMAKE_VERSION} VERSION_LESS 3.14" OFF)
cmake_dependent_option(ENABLE_COZ "Setup Coz, causal profiling, for use with this project." OFF "UNIX" OFF)

_set_configs(_compiler_explorer_configs compiler_explorer)
_set_configs(_coz_configs coz)

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

function(_try_package name target)
	set(_options)
	set(_single_values)
	set(_multi_values DEPENDS ALL_FILES ONE_LIB)
	cmake_parse_arguments(_parsed "${_options}" "${_single_values}" "${_multi_values}" ${ARGN})

	cmake_parse_arguments(_parsed_ALL_FILES "" "" "IN" ${_parsed_ALL_FILES})
	set(_parsed_ALL_FILES ${_parsed_ALL_FILES_UNPARSED_ARGUMENTS})

	string(TOUPPER "${name}" _big_name)

	set(_valid True)
	foreach(_config IN ITEMS ${_${name}_configs})
		string(TOUPPER "${_config}" _big_config)

		if(ENABLE_${_big_name})
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
		foreach(_config IN ITEMS ${_${name}_configs})
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

_try_package(coz dep_coz ALL_FILES coz.h)
