if(NOT ${CMAKE_VERSION} VERSION_LESS 3.14)
	include(FetchContent)
endif()

# options

cmake_dependent_option(ENABLE_COMPILER_EXPLORER "Setup Compiler Explorer for use with this project." OFF "NOT ${CMAKE_VERSION} VERSION_LESS 3.14" OFF)
cmake_dependent_option(ENABLE_COZ "Setup Coz, causal profiling, for use with this project." OFF "UNIX" OFF)

# build dependencies

if(ENABLE_COZ)
	# https://github.com/aclements/libelfin
	macro(_package_libelfin)
		# note packaging this library is not necessary because coz will
		# find it by itself but we pack it anyway because then we know if
		# we need to build it or not
		dep_package(libelfin fiw_libelfin
			${ARGV}

			LIBS
			"dwarf++" "elf++"
			)
	endmacro()

	cmake_dependent_option(BUILD_LIBELFIN "Build libelfin" ON "ENABLE_COZ" OFF)
	if(BUILD_LIBELFIN)
		_package_libelfin(IGNORE_SYSTEM)

		dep_build(libelfin
			FIND_GIT

			DOWNLOAD_STEP
			GIT_REPOSITORY "https://github.com/aclements/libelfin.git"
			GIT_TAG "ac45a094fadba77ad840063fb7aab82571546be0"

			UPDATE_STEP
			PATCH_COMMAND "\$\{GIT_EXECUTABLE\}" apply --index "${PROJECT_SOURCE_DIR}/.patch/libelfin-const.patch"

			CONFIGURE_STEP
			CONFIGURE_COMMAND ""

			BUILD_STEP
			BUILD_COMMAND "CXX=${CMAKE_CXX_COMPILER}" make
			BUILD_IN_SOURCE True

			INSTALL_STEP
			INSTALL_COMMAND "PREFIX=\$\{_install_prefix\}" make install
			)
	endif()

	_package_libelfin()

	#
	macro(_package_coz)
		dep_package(coz fiw_coz
			${ARGV}

			FILES
			coz.h
			)
	endmacro()

	cmake_dependent_option(BUILD_COZ "Build coz" ON "ENABLE_COZ" OFF)
	if(BUILD_COZ)
		_package_coz(IGNORE_SYSTEM)

		try_compile(
			FIW_PROBE_GETTID_RESULT

			"${PROJECT_BINARY_DIR}/probe/gettid"
			"${PROJECT_SOURCE_DIR}/.probe/gettid.c"

			CMAKE_FLAGS "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
			)

		if(FIW_PROBE_GETTID_RESULT)
			set(_update_step UPDATE_STEP UPDATE_COMMAND "")
		else()
			set(_update_step UPDATE_STEP PATCH_COMMAND "\$\{GIT_EXECUTABLE\}" apply --index "${PROJECT_SOURCE_DIR}/.patch/coz-gettid.patch")
		endif()

		dep_build(coz
			FIND_GIT

			DOWNLOAD_STEP
			GIT_REPOSITORY "https://github.com/plasma-umass/coz.git"
			GIT_TAG "96b45326ad61333161decf4173192930d89f2fbd"

			${_update_step}

			BUILD_STEP
			BUILD_COMMAND "PKG_CONFIG_LIBDIR=\$\{_install_prefix\}/lib/pkgconfig" "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}" make -C "\$\{_source_dir\}"

			INSTALL_STEP
			INSTALL_COMMAND "PKG_CONFIG_LIBDIR=\$\{_install_prefix\}/lib/pkgconfig" "prefix=\$\{_install_prefix\}" make -C "\$\{_source_dir\}" install
			)
	endif()

	_package_coz()
endif()

# setup

cmake_dependent_option(BUILD_COMPILER_EXPLORER "Build compiler explorer" ON "ENABLE_COMPILER_EXPLORER" OFF)
if(BUILD_COMPILER_EXPLORER)
	FetchContent_Declare(
		compilerexplorer

		SOURCE_DIR "${PROJECT_SOURCE_DIR}/tools/compiler-explorer"

		GIT_REPOSITORY "https://github.com/shossjer/compiler-explorer.git"
		GIT_TAG "project-browsing"
		)
	FetchContent_GetProperties(compilerexplorer)
	if(NOT compilerexplorer_POPULATED)
		FetchContent_Populate(compilerexplorer)
	endif()
endif()

if(ENABLE_COMPILER_EXPLORER)
	file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-compiler-explorer")
	file(TOUCH "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-compiler-explorer/codemodel-v2")

	# todo the location of compiler-explorer should be customizable and
	# only set to "<source>/tools/compiler-explorer" if it was built
	get_filename_component(_binary_dir_name "${CMAKE_BINARY_DIR}" NAME)
	file(RELATIVE_PATH _binary_dir_path "${PROJECT_SOURCE_DIR}/tools/compiler-explorer" "${CMAKE_BINARY_DIR}")

	file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/tools/compiler-explorer/projects")
	file(WRITE "${PROJECT_SOURCE_DIR}/tools/compiler-explorer/projects/${_binary_dir_name}" "${_binary_dir_path}")

	unset(_binary_dir_path)
	unset(_binary_dir_name)
else()
	get_filename_component(_binary_dir_name "${CMAKE_BINARY_DIR}" NAME)

	file(REMOVE "${PROJECT_SOURCE_DIR}/tools/compiler-explorer/projects/${_binary_dir_name}")

	unset(_binary_dir_name)

	file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-compiler-explorer")
endif()

if(TARGET fiw_coz)
	get_property(_dep_directory DIRECTORY PROPERTY DEP_DIRECTORY)
	if(BUILD_COZ)
		find_program(FIW_COZ_EXECUTABLE coz PATHS "${_dep_directory}/Release/bin" NO_DEFAULT_PATH)
		set(_executable "LD_LIBRARY_PATH=${_dep_directory}/Release/lib ${FIW_COZ_EXECUTABLE}")
	else()
		find_program(FIW_COZ_EXECUTABLE coz)
		set(_executable "${FIW_COZ_EXECUTABLE}")
	endif()

	configure_file(
		"${PROJECT_SOURCE_DIR}/tools/coz/run.in"
		"${PROJECT_SOURCE_DIR}/tools/coz/run"
		@ONLY
		)
endif()
