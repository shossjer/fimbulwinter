cmake_minimum_required(VERSION 3.5.1)

include(CMakeDependentOption)
include(dep/dep.cmake)

project(fimbulwinter CXX)

# project setup

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

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 14)
	message(STATUS "C++ standard ${CMAKE_CXX_STANDARD} (default)")
elseif(CMAKE_CXX_STANDARD LESS 14)
	message(FATAL_ERROR "Require C++ standard >= 14")
else()
	message(STATUS "C++ standard ${CMAKE_CXX_STANDARD}")
endif()
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_property(_dep_directory_set DIRECTORY PROPERTY "DEP_DIRECTORY" SET)
if(NOT _dep_directory_set)
	set_property(DIRECTORY PROPERTY DEP_DIRECTORY "${CMAKE_SOURCE_DIR}/dep/${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
endif()

# project information

message(STATUS "Using CXX compiler ${CMAKE_CXX_COMPILER}")
message(STATUS "System is ${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")

#

add_subdirectory(fio)
add_subdirectory(ful)
add_subdirectory(tools)

# build dependencies

macro(_package_catch)
	dep_package(catch fiw_catch
		${ARGV}

		FILES
		catch2/catch.hpp
		)
endmacro()

option(FIW_BUILD_CATCH "Build catch" ON)
if(FIW_BUILD_CATCH)
	_package_catch(IGNORE_SYSTEM)

	dep_build(catch
		DOWNLOAD_STEP
		GIT_REPOSITORY "https://github.com/philsquared/Catch.git"
		GIT_TAG "v2.12.1"
		GIT_SHALLOW True

		CMAKE_OPTIONS
		-DCATCH_BUILD_TESTING=OFF
		-DCATCH_INSTALL_DOCS=OFF
		-DCATCH_INSTALL_HELPERS=OFF
		)
endif()

_package_catch()

# build settings

cmake_dependent_option(FIW_BUILD_TESTS "Build tests" ON "TARGET fiw_catch" OFF)
add_library(fiw_test INTERFACE)
if(FIW_BUILD_TESTS)
	target_link_libraries(fiw_test INTERFACE fiw_catch)
	# target_compile_options(fiw_test INTERFACE
	# 	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
	# 		-Wno-sign-compare -Wno-sign-conversion>
	# 	$<$<CXX_COMPILER_ID:Clang>:
	# 		-Wno-self-move>
	# 	$<$<CXX_COMPILER_ID:MSVC>:
	# 		/wd4389 /wd4456>
	# 	)
endif()

cmake_dependent_option(FIW_TESTS_BUILDTIME "Run tests as part of the build process." ON "FIW_BUILD_TESTS" OFF)

cmake_dependent_option(FIW_BUILD_BENCHMARKS "Build benchmarks" OFF "TARGET fiw_catch" OFF)
add_library(fiw_benchmark INTERFACE)
if(FIW_BUILD_BENCHMARKS)
	target_link_libraries(fiw_benchmark INTERFACE fiw_catch)
	target_compile_definitions(fiw_benchmark INTERFACE CATCH_CONFIG_ENABLE_BENCHMARKING)
endif()

# build features

#
macro(_package_khrplatform)
	dep_package(khrplatform fiw_khrplatform
		${ARGV}

		FILES
		KHR/khrplatform.h
		)
endmacro()

option(FIW_BUILD_KHRPLATFORM "Build khrplatform" ON)
if(FIW_BUILD_KHRPLATFORM)
	_package_khrplatform(IGNORE_SYSTEM)

	dep_build(khrplatform
		DOWNLOAD_STEP
		URL "https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h"
		DOWNLOAD_NO_EXTRACT True
		TIMEOUT 10

		INSTALL_STEP
		INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "\$\{_install_prefix\}/include/KHR"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "\$\{_download_dir\}/khrplatform.h" "\$\{_install_prefix\}/include/KHR"
		)
endif()

_package_khrplatform()

#
macro(_package_glext)
	dep_package(glext fiw_glext
		${ARGV}

		DEPENDS
		fiw_khrplatform

		FILES
		GL/glext.h
		)
endmacro()

option(FIW_BUILD_GLEXT "Build glext" ON)
if(FIW_BUILD_GLEXT)
	_package_glext(IGNORE_SYSTEM)

	dep_build(glext
		DOWNLOAD_STEP
		URL "https://www.khronos.org/registry/OpenGL/api/GL/glext.h"
		DOWNLOAD_NO_EXTRACT True
		TIMEOUT 10

		INSTALL_STEP
		INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "\$\{_install_prefix\}/include/GL"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "\$\{_download_dir\}/glext.h" "\$\{_install_prefix\}/include/GL"
		)
endif()

_package_glext()

#
macro(_package_zlib)
	dep_package(zlib fiw_zlib
		SUPPORTS_DEBUG
		${ARGV}

		FILES
		zlib.h

		LIBS
		Linux_x86_64
		libz.a
		Windows_AMD64
		optimized zlibstatic
		debug zlibstaticd
		)
endmacro()

option(FIW_BUILD_ZLIB "Build zlib" ON)
if(FIW_BUILD_ZLIB)
	_package_zlib(IGNORE_SYSTEM)

	dep_build(zlib
		SUPPORTS_DEBUG

		DOWNLOAD_STEP
		URL "https://zlib.net/zlib-1.2.11.tar.xz"
		URL_HASH "SHA256=4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066"
		TIMEOUT 10

		UPDATE_STEP
		Windows_AMD64
		PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/.patch/zlib-cache.patch" "\$\{_source_dir\}/CMakeLists.txt"

		CMAKE_OPTIONS
		-DBUILD_SHARED_LIBS=OFF
		-DSKIP_INSTALL_FILES=ON
		)
endif()

_package_zlib()

#
macro(_package_libpng)
	dep_package(libpng fiw_libpng
		SUPPORTS_DEBUG
		${ARGV}

		DEPENDS
		fiw_zlib

		FILES
		libpng16/png.h

		LIBS
		Linux_x86_64
		optimized libpng16.a
		debug libpng16d.a
		Windows_AMD64
		optimized libpng16_static
		debug libpng16_staticd
		)
endmacro()

option(FIW_BUILD_LIBPNG "Build libpng" ON)
if(FIW_BUILD_LIBPNG)
	_package_libpng(IGNORE_SYSTEM)

	dep_build(libpng
		SUPPORTS_DEBUG

		DOWNLOAD_STEP
		URL "ftp://ftp-osl.osuosl.org/pub/libpng/src/archive/xz/libpng16/libpng-1.6.30.tar.xz"
		URL_HASH "SHA256=267c332ffff70cc599d3929207869f698798f1df143aa5f9597b007c14353666"
		TIMEOUT 10

		CMAKE_OPTIONS
		-Wno-dev
		-DPNG_TESTS=OFF
		-DSKIP_INSTALL_EXECUTABLES=ON
		-DSKIP_INSTALL_PROGRAMS=ON
		-DSKIP_INSTALL_FILES=ON
		-DSKIP_INSTALL_EXPORT=ON
		)
endif()

_package_libpng()

#
macro(_package_freetype)
	dep_package(freetype fiw_freetype
		SUPPORTS_DEBUG
		${ARGV}

		DEPENDS
		fiw_zlib
		fiw_libpng

		FILES
		IN_DIRECTORY freetype2
		ft2build.h

		LIBS
		optimized freetype
		debug freetyped
		)
endmacro()

option(FIW_BUILD_FREETYPE "Build freetype" ON)
if(FIW_BUILD_FREETYPE)
	_package_freetype(IGNORE_SYSTEM)

	dep_build(freetype
		SUPPORTS_DEBUG

		DOWNLOAD_STEP
		URL "https://download.savannah.gnu.org/releases/freetype/freetype-2.8.1.tar.gz"
		URL_HASH "SHA256=876711d064a6a1bd74beb18dd37f219af26100f72daaebd2d86cb493d7cd7ec6"
		TIMEOUT 10

		CMAKE_OPTIONS
		-DWITH_ZLIB=${FIW_BUILD_ZLIB}
		-DWITH_PNG=${FIW_BUILD_LIBPNG}
		-DWITH_BZip2=OFF
		-DWITH_HarfBuzz=OFF
		)
endif()

_package_freetype()

#
dep_package(fmod fiw_fmod
	FILES fmod.hpp
	LIBS
	Linux_x86_64 fmod
	Windows_AMD64 fmod_vc
	)

# lookup dependencies

# BSD
message(STATUS "Looking for BSD")
find_file(FILE_SYS_FILE_H sys/file.h)
message(STATUS "Looking for BSD file - ${FILE_SYS_FILE_H}")

# Linux
message(STATUS "Looking for Linux")
find_file(FILE_SYS_INOTIFY_H sys/inotify.h)
message(STATUS "Looking for Linux inotify - ${FILE_SYS_INOTIFY_H}")

# OpenGL
message(STATUS "Looking for OpenGL")
find_file(FILE_GL_GL_H GL/gl.h)
message(STATUS "Looking for OpenGL - ${FILE_GL_GL_H}")
find_library(LIBRARY_GL GL)
find_library(LIBRARY_OPENGL32 opengl32)
message(STATUS "Looking for OpenGL - ${LIBRARY_GL}")
message(STATUS "Looking for OpenGL - ${LIBRARY_OPENGL32}")

# POSIX
message(STATUS "Looking for POSIX")
find_file(FILE_DIRENT_H dirent.h)
message(STATUS "Looking for POSIX dirent - ${FILE_DIRENT_H}")
find_file(FILE_FCNTL_H fcntl.h)
message(STATUS "Looking for POSIX fcntl - ${FILE_FCNTL_H}")
find_file(FILE_FTW_H ftw.h)
message(STATUS "Looking for POSIX ftw - ${FILE_FTW_H}")
find_file(FILE_POLL_H poll.h)
message(STATUS "Looking for POSIX poll - ${FILE_POLL_H}")
find_file(FILE_PTHREAD_H pthread.h)
message(STATUS "Looking for POSIX pthread - ${FILE_PTHREAD_H}")
find_library(LIBRARY_PTHREAD pthread)
message(STATUS "Looking for POSIX pthread - ${LIBRARY_PTHREAD}")
find_file(FILE_SYS_STAT_H sys/stat.h)
message(STATUS "Looking for POSIX stat - ${FILE_SYS_STAT_H}")
find_file(FILE_UNISTD_H unistd.h)
message(STATUS "Looking for POSIX unistd - ${FILE_UNISTD_H}")

# Windows
message(STATUS "Looking for Windows")
find_file(FILE_FILEAPI_H fileapi.h)
message(STATUS "Looking for Windows fileapi - ${FILE_FILEAPI_H}")
find_file(FILE_HIDSDI_H Hidsdi.h)
message(STATUS "Looking for Windows Hidsdi - ${FILE_HIDSDI_H}")
find_library(LIBRARY_GDI32 gdi32)
message(STATUS "Looking for Windows Gdi32 - ${LIBRARY_GDI32}")
find_library(LIBRARY_HID Hid)
message(STATUS "Looking for Windows Hid - ${LIBRARY_HID}")
find_library(LIBRARY_KERNEL32 kernel32)
message(STATUS "Looking for Windows Kernel32 - ${LIBRARY_KERNEL32}")
find_library(LIBRARY_USER32 user32)
message(STATUS "Looking for Windows User32 - ${LIBRARY_USER32}")

# X11
message(STATUS "Looking for X11")
find_file(FILE_X11_XLIB_H X11/Xlib.h)
message(STATUS "Looking for X11 - ${FILE_X11_XLIB_H}")
find_library(LIBRARY_X11 X11)
message(STATUS "Looking for X11 - ${LIBRARY_X11}")

# module dependencies

# usage:
#
#   _resolve_options(options... [| options...]*)
#
# description:
#
#   generates an error if more than one option is set within a section
#   (separated by "|"), turns off all options from remaining sections
#   (not including the section with the first set option) and
#   generates a warning in the process
macro(_resolve_options)
	set(_resolve_options_chosen "")
	set(_resolve_options_clear_remaining "")

	foreach(_arg IN ITEMS ${ARGV})
		if(_arg MATCHES "\\|")
			set(_resolve_options_clear_remaining ${_resolve_options_chosen})
		elseif(${_arg})
			if(_resolve_options_clear_remaining)
				message(WARNING "option ${_arg} is overridden by ${_resolve_options_chosen}")
				set(${_arg} OFF)
			elseif(_resolve_options_chosen)
				message(FATAL_ERROR "conflicting options ${_resolve_options_chosen} and ${_arg}")
			else()
				set(_resolve_options_chosen ${_arg})
			endif()
		endif()
	endforeach()

	unset(_resolve_options_chosen)
	unset(_resolve_options_clear_remaining)
endmacro()

# todo remove
add_library(fiw_utility_features INTERFACE)
add_library(fiw_core_features INTERFACE)
add_library(fiw_engine_features INTERFACE)

if(TARGET fiw_libpng)
	target_link_libraries(fiw_core_features INTERFACE fiw_libpng)
	target_compile_definitions(fiw_core_features INTERFACE FIW_HAVE_LIBPNG)
endif()

if(TARGET fiw_zlib)
	target_link_libraries(fiw_core_features INTERFACE fiw_zlib)
	target_compile_definitions(fiw_core_features INTERFACE FIW_HAVE_ZLIB)
endif()

# audio
cmake_dependent_option(AUDIO_USE_FMOD "Use the FMOD library for the audio module" ON "TARGET fiw_fmod" OFF)
add_library(audio_dep INTERFACE)
if(AUDIO_USE_FMOD)
	message(STATUS "Audio - FMOD")
	target_link_libraries(audio_dep INTERFACE fiw_fmod)
else()
	message(WARNING "Audio - Disabled")
	set(AUDIO_USE_DUMMY 1)
endif()

# console
if (FILE_POLL_H AND
	 FILE_UNISTD_H)
  message(STATUS "Console - POSIX")
  set(CONSOLE_USE_POSIX 1)
elseif (LIBRARY_KERNEL32)
  message(STATUS "Console - Kernel32")
  set(CONSOLE_USE_KERNEL32 1)
else ()
  message(WARNING "Console - Disabled")
endif ()

# file
cmake_dependent_option(FILE_SYSTEM_USE_POSIX "Use the POSIX API for the file system module" ON "FILE_DIRENT_H AND FILE_FCNTL_H AND FILE_FTW_H AND FILE_POLL_H AND FILE_SYS_FILE_H AND FILE_SYS_STAT_H AND FILE_UNISTD_H" OFF)
cmake_dependent_option(FILE_SYSTEM_USE_KERNEL32 "Use the Windows file API for the file system module" ON "FILE_FILEAPI_H AND LIBRARY_KERNEL32" OFF)
_resolve_options(FILE_SYSTEM_USE_POSIX FILE_SYSTEM_USE_KERNEL32)
if(FILE_SYSTEM_USE_POSIX)
	message(STATUS "File System - POSIX")
elseif(FILE_SYSTEM_USE_KERNEL32)
	message(STATUS "File System - Kernel32")
else()
	message(WARNING "File System - Disabled")
	set(FILE_SYSTEM_USE_DUMMY 1)
endif()

cmake_dependent_option(FILE_WATCH_USE_INOTIFY "Use the Linux inotify API for the file watch sub module" ON "FILE_SYSTEM_USE_POSIX AND FILE_SYS_INOTIFY_H" OFF)
cmake_dependent_option(FILE_WATCH_USE_KERNEL32 "Use the Windows file API for the file watch sub module" ON "FILE_SYSTEM_USE_KERNEL32" OFF)
_resolve_options(FILE_WATCH_USE_INOTIFY FILE_WATCH_USE_KERNEL32)
if(FILE_WATCH_USE_INOTIFY)
	message(STATUS "File Watch - Inotify")
elseif(FILE_WATCH_USE_KERNEL32)
	message(STATUS "File Watch - Kernel32")
else()
	message(WARNING "File Watch - Disabled")
	set(FILE_WATCH_USE_DUMMY 1)
endif()

# graphics
if(FILE_GL_GL_H AND TARGET fiw_glext AND (LIBRARY_GL OR LIBRARY_OPENGL32))
	set(FOUND_OPENGL 1)
endif()
# hack to simplify the expression for `cmake_dependent_option`
cmake_dependent_option(GRAPHICS_USE_OPENGL "Use OpenGL for the graphics module" ON "FOUND_OPENGL" OFF)
add_library(graphics_dep INTERFACE)
if(GRAPHICS_USE_OPENGL)
	message(STATUS "Graphics - OpenGL")

	target_link_libraries(graphics_dep INTERFACE fiw_khrplatform fiw_glext)
	if(LIBRARY_GL)
		target_link_libraries(graphics_dep INTERFACE "${LIBRARY_GL}")
	else()
		target_link_libraries(graphics_dep INTERFACE "${LIBRARY_OPENGL32}")
	endif()
else()
	message(WARNING "Graphics - Disabled")
	set(GRAPHICS_USE_DUMMY 1)
endif()

# window
cmake_dependent_option(WINDOW_USE_USER32 "Use the Windows API for the window module" ON "LIBRARY_GDI32 AND LIBRARY_USER32 AND GRAPHICS_USE_OPENGL" OFF)
cmake_dependent_option(WINDOW_USE_X11 "Use X11/Xlib for the window module" ON "FILE_X11_XLIB_H AND LIBRARY_X11 AND GRAPHICS_USE_OPENGL" OFF)
_resolve_options(WINDOW_USE_USER32 WINDOW_USE_X11)
add_library(window_dep INTERFACE)
if(WINDOW_USE_USER32)
	message(STATUS "Window - User32")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_USER32}")
elseif(WINDOW_USE_X11)
	message(STATUS "Window - X11")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_X11}")
else()
	message(WARNING "Window - Disabled")
      set(WINDOW_USE_DUMMY 1)
endif()

# additional libraries

# input
cmake_dependent_option(INPUT_HAS_USER32_HID "Enable HID in the input module" ON "WINDOW_USE_USER32 AND FILE_HIDSDI_H AND LIBRARY_HID" OFF)
add_library(input_dep INTERFACE)
if(INPUT_HAS_USER32_HID)
	message(STATUS "Input - User32 Hid")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_HID}")
endif()

# task
message(STATUS "Task - Dummy (single thread)")
set(TASK_USE_DUMMY 1)

# text
cmake_dependent_option(TEXT_USE_FREETYPE "Use Freetype as text/font source" ON "TARGET fiw_freetype" OFF)
cmake_dependent_option(TEXT_USE_USER32 "Use the Windows API as text/font source" ON "WINDOW_USE_USER32" OFF)
cmake_dependent_option(TEXT_USE_X11 "Use X11/Xlib as text/font source" ON "WINDOW_USE_X11" OFF)
_resolve_options(TEXT_USE_FREETYPE | TEXT_USE_USER32 TEXT_USE_X11)
add_library(text_dep INTERFACE)
if(TEXT_USE_FREETYPE)
	message(STATUS "Text - Freetype")

	target_link_libraries(text_dep INTERFACE fiw_freetype)
elseif(TEXT_USE_USER32)
	message(WARNING "Text - User32")
elseif(TEXT_USE_X11)
	message(WARNING "Text - X11")
else()
	message(WARNING "Text - Disabled")
endif()

# thread
cmake_dependent_option(THREAD_USE_KERNEL32 "Use the Windows API for threads" ON "LIBRARY_KERNEL32" OFF)
cmake_dependent_option(THREAD_USE_PTHREAD "Use POSIX for threads" ON "FILE_PTHREAD_H AND LIBRARY_PTHREAD" OFF)
_resolve_options(THREAD_USE_KERNEL32 THREAD_USE_PTHREAD)
add_library(thread_dep INTERFACE)
if(THREAD_USE_KERNEL32)
	message(STATUS "Thread - Kernel32")

	target_link_libraries(thread_dep INTERFACE "${LIBRARY_KERNEL32}")
elseif(THREAD_USE_PTHREAD)
	message(STATUS "Thread - Pthread")

	target_link_libraries(thread_dep INTERFACE "${LIBRARY_PTHREAD}")
else()
	message(FATAL_ERROR "Thread - Disabled")
endif()

# lookup features

cmake_dependent_option(PROFILING_COZ "Enable Coz profiling" OFF "TARGET dep_coz" OFF)
add_library(fiw_profiling INTERFACE)
if(PROFILING_COZ)
	message(STATUS "Enabled Coz profiling")
	target_link_libraries(fiw_profiling INTERFACE dep_coz)
	target_compile_definitions(fiw_profiling INTERFACE
		PROFILING_COZ=1
		)
	target_compile_options(fiw_profiling INTERFACE
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			-g>
		)
else()
	message(STATUS "Disabled profiling")
endif()

if(CMAKE_BUILD_TYPE)
	string(TOUPPER "${CMAKE_BUILD_TYPE}" _upper_build_type)
	set(MODE_${_upper_build_type} 1)
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/generated/config.h"
  @ONLY
  )

add_library(generated INTERFACE)
target_include_directories(generated INTERFACE "${PROJECT_BINARY_DIR}/generated")

add_subdirectory(code)
