cmake_minimum_required(VERSION 3.7)

# 3.12 string(JOIN ...)
# except this function quotes all arguments
function(_join glue outvar)
	set(_str "")
	foreach(_arg IN LISTS ARGN)
		if(_str)
			set(_str "${_str}${glue}\"${_arg}\"")
		else()
			set(_str "\"${_arg}\"")
		endif()
	endforeach()
	set(${outvar} "${_str}" PARENT_SCOPE)
endfunction()

function(_build_dependency dependency)
	set(_options FIND_GIT)
	set(_multi_values BUILD_STEP CMAKE_OPTIONS CONFIGS CONFIGURE_STEP DOWNLOAD_STEP INSTALL_STEP UPDATE_STEP)
	cmake_parse_arguments(PARSE_ARGV 0 _parsed "${_options}" "" "${_multi_values}")
	# note we have to use the 3.7 syntax here in order to properly
	# handle empty strings as arguments, read the discussion at
	# https://gitlab.kitware.com/cmake/cmake/-/issues/16341
	# for more details

	set(_download_step "\"DOWNLOAD_COMMAND\" \"\"")
	set(_update_step "\"UPDATE_COMMAND\" \"\"")
	set(_configure_step "\"CONFIGURE_COMMAND\" \"\"")
	set(_build_step "\"BUILD_COMMAND\" \"\"")
	set(_install_step "\"INSTALL_COMMAND\" \"\"")

	if(DEFINED _parsed_DOWNLOAD_STEP)
		_join(" " _download_step "${_parsed_DOWNLOAD_STEP}")
	endif()

	if(DEFINED _parsed_UPDATE_STEP)
		_join(" " _update_step "${_parsed_UPDATE_STEP}")
	endif()

	if(DEFINED _parsed_CONFIGURE_STEP)
		_join(" " _configure_step "${_parsed_CONFIGURE_STEP}")
	endif()

	if(DEFINED _parsed_BUILD_STEP)
		_join(" " _build_step "${_parsed_BUILD_STEP}")
	endif()

	if(DEFINED _parsed_INSTALL_STEP)
		_join(" " _install_step "${_parsed_INSTALL_STEP}")
	endif()

	if(DEFINED _parsed_CMAKE_OPTIONS)
		_join(" " _configure_step CONFIGURE_COMMAND \$\{CMAKE_COMMAND\} -C "${_initial_cache_file}" -DCMAKE_PREFIX_PATH=\$\{_install_prefix\} -DCMAKE_INSTALL_PREFIX=\$\{_install_prefix\} "${_parsed_CMAKE_OPTIONS}" \$\{_source_dir\})
		set(_build_step "")
		set(_install_step "")
	endif()

	set(_build_dir "${PROJECT_BINARY_DIR}/generated/${dependency}")

	configure_file(
		"${PROJECT_SOURCE_DIR}/CMakeLists.txt.in"
		"${_build_dir}/CMakeLists.txt"
		@ONLY
		)

	if(DEFINED CMAKE_CONFIGURATION_TYPES)
		execute_process(
			COMMAND ${CMAKE_COMMAND} "-DCMAKE_CONFIGURATION_TYPES=${_parsed_CONFIGS}" .
			WORKING_DIRECTORY ${_build_dir}
			)

		foreach(_config IN LISTS _parsed_CONFIGS)
			execute_process(
				COMMAND ${CMAKE_COMMAND} --build . --config ${_config}
				WORKING_DIRECTORY ${_build_dir}
				)
		endforeach()
	elseif(${CMAKE_BUILD_TYPE} IN_LIST _parsed_CONFIGS)
		execute_process(
			COMMAND ${CMAKE_COMMAND} . -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
			WORKING_DIRECTORY ${_build_dir}
			)

		execute_process(
			COMMAND ${CMAKE_COMMAND} --build .
			WORKING_DIRECTORY ${_build_dir}
			)
	endif()
endfunction()

macro(_set_configs outvar)
	if(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "SUPPORTS_DEBUG" AND DEP_DEBUG_CONFIG)
		set(${outvar} ${DEP_RELEASE_CONFIG} ${DEP_DEBUG_CONFIG})
	else()
		set(${outvar} ${DEP_RELEASE_CONFIG})
	endif()
endmacro()

project(dep NONE)

set(DEP_RELEASE_CONFIG Release CACHE STRING "Configuration type to use for dependencies.")
set(DEP_DEBUG_CONFIG "" CACHE STRING "Configuration type to use for dependencies, in debug. Leave empty to disable debug builds.")

if("${DEP_DEBUG_CONFIG}" STREQUAL "${DEP_RELEASE_CONFIG}")
	message(WARNING "DEP_DEBUG_CONFIG equals DEP_RELEASE_CONFIG, will ignore it")
	set(DEP_DEBUG_CONFIG "")
endif()

if(DEFINED CMAKE_CONFIGURATION_TYPES)
	if(DEP_DEBUG_CONFIG)
		set(CMAKE_CONFIGURATION_TYPES ${DEP_RELEASE_CONFIG} ${DEP_DEBUG_CONFIG})
	else()
		set(CMAKE_CONFIGURATION_TYPES ${DEP_RELEASE_CONFIG})
	endif()
else()
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

_set_configs(_catch_configs)
_set_configs(_freetype_configs SUPPORTS_DEBUG)
_set_configs(_glext_configs)
_set_configs(_json_configs)
_set_configs(_libpng_configs SUPPORTS_DEBUG)
_set_configs(_zlib_configs SUPPORTS_DEBUG)

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

set(_timeout 20)

if(BUILD_CATCH)
	_build_dependency(catch
		CONFIGS ${_catch_configs}

		DOWNLOAD_STEP
		GIT_REPOSITORY "https://github.com/philsquared/Catch.git"
		GIT_TAG "Catch1.x"

		CMAKE_OPTIONS
		-DCATCH_BUILD_TESTING=OFF
		-DCATCH_INSTALL_DOCS=OFF
		-DCATCH_INSTALL_HELPERS=OFF
		)
endif()

if(BUILD_GLEXT)
	_build_dependency(khrplatform
		CONFIGS ${_glext_configs}

		DOWNLOAD_STEP
		URL "https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h"
		DOWNLOAD_NO_EXTRACT True
		TIMEOUT ${_timeout}

		INSTALL_STEP
		INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "\$\{_install_prefix\}/include/KHR"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "\$\{_download_dir\}/khrplatform.h" "\$\{_install_prefix\}/include/KHR"
		)

	_build_dependency(glext
		CONFIGS ${_glext_configs}

		DOWNLOAD_STEP
		URL "https://www.khronos.org/registry/OpenGL/api/GL/glext.h"
		DOWNLOAD_NO_EXTRACT True
		TIMEOUT ${_timeout}

		INSTALL_STEP
		INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "\$\{_install_prefix\}/include/GL"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "\$\{_download_dir\}/glext.h" "\$\{_install_prefix\}/include/GL"
		)
endif()

if(BUILD_JSON)
	_build_dependency(nlohmann-json
		CONFIGS ${_json_configs}

		DOWNLOAD_STEP
		GIT_REPOSITORY "https://github.com/nlohmann/json.git"
		GIT_TAG "ce0b3fe5a334567825a554a08b76c725d0790500"

		CMAKE_OPTIONS
		-DBuildTests=OFF
		)
endif()

if(BUILD_ZLIB)
	_build_dependency(zlib
		CONFIGS ${_zlib_configs}

		DOWNLOAD_STEP
		URL "https://zlib.net/zlib-1.2.11.tar.xz"
		URL_HASH "SHA256=4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066"
		TIMEOUT ${_timeout}

		CMAKE_OPTIONS
		-DSKIP_INSTALL_FILES=ON
		)
endif()

if(BUILD_LIBPNG)
	_build_dependency(libpng
		CONFIGS ${_libpng_configs}

		DOWNLOAD_STEP
		URL "ftp://ftp-osl.osuosl.org/pub/libpng/src/archive/xz/libpng16/libpng-1.6.30.tar.xz"
		URL_HASH "SHA256=267c332ffff70cc599d3929207869f698798f1df143aa5f9597b007c14353666"
		TIMEOUT ${_timeout}

		CMAKE_OPTIONS
		-Wno-dev
		-DPNG_TESTS=OFF
		-DSKIP_INSTALL_EXECUTABLES=ON
		-DSKIP_INSTALL_PROGRAMS=ON
		-DSKIP_INSTALL_FILES=ON
		-DSKIP_INSTALL_EXPORT=ON
		)
endif()

if(BUILD_FREETYPE)
	_build_dependency(freetype
		CONFIGS ${_freetype_configs}

		DOWNLOAD_STEP
		URL "https://download.savannah.gnu.org/releases/freetype/freetype-2.8.1.tar.gz"
		URL_HASH "SHA256=876711d064a6a1bd74beb18dd37f219af26100f72daaebd2d86cb493d7cd7ec6"
		TIMEOUT ${_timeout}

		CMAKE_OPTIONS
		-DWITH_ZLIB=${BUILD_ZLIB}
		-DWITH_PNG=${BUILD_LIBPNG}
		-DWITH_BZip2=OFF
		-DWITH_HarfBuzz=OFF
		)
endif()

function(_try_package name target)
	set(_multi_values DEPENDS ALL_FILES ONE_LIB)
	cmake_parse_arguments(_parsed "" "" "${_multi_values}" ${ARGN})

	cmake_parse_arguments(_parsed_ALL_FILES "" "" "IN" ${_parsed_ALL_FILES})
	set(_parsed_ALL_FILES ${_parsed_ALL_FILES_UNPARSED_ARGUMENTS})

	string(TOUPPER "${name}" _big_name)

	set(_valid True)
	foreach(_config IN ITEMS ${_${name}_configs})
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

_try_package(catch dep_catch ALL_FILES catch2/catch.hpp)
_try_package(glext dep_glext ALL_FILES GL/glext.h KHR/khrplatform.h)
_try_package(json dep_json ALL_FILES nlohmann/json.hpp)
_try_package(zlib dep_zlib ALL_FILES zlib.h ONE_LIB libz.a zlib)

_try_package(libpng dep_libpng DEPENDS dep_zlib ALL_FILES libpng16/png.h ONE_LIB libpng16.a libpng16_static)

_try_package(freetype dep_freetype DEPENDS dep_zlib dep_libpng ALL_FILES ft2build.h IN freetype2 ONE_LIB freetype)
