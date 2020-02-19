cmake_minimum_required(VERSION 3.5.1)

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

project(fimbulwinter C CXX)

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

add_subdirectory(dep)

# todo remove
set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH};${PROJECT_SOURCE_DIR}/dep/include")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${PROJECT_SOURCE_DIR}/dep/lib")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# todo move into code
set(private_compile_options
	$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
		-Wall -Wextra -Wconversion -Werror -Wno-sign-conversion>
	# sign-conversion - Warn for implicit conversions that may change
	# the sign of an integer value, like assigning a signed integer
	# expression to an unsigned integer variable.
	$<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_LESS:CXX_COMPILER_VERSION,6.0>>:
		-Wno-missing-braces>
	# missing-braces - Warn if an aggregate or union initializer is not
	# fully bracketed.
	$<$<CXX_COMPILER_ID:MSVC>:
		/W4 /WX>
	)

set(private_compile_definitions
	$<$<CXX_COMPILER_ID:MSVC>:
		NOMINMAX UNICODE _UNICODE>
	)

# project information
message(STATUS "Using CXX compiler ${CMAKE_CXX_COMPILER}")
message("")

message(STATUS "CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
message(STATUS "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")

message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_SYSTEM_PREFIX_PATH=${CMAKE_SYSTEM_PREFIX_PATH}")
message(STATUS "CMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}")
message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")
message("")

# hardware settings
include(TestBigEndian)
test_big_endian(ENDIANESS)
if (ENDIANESS)
  set(BIG_WORDS 1)
endif ()

# lookup dependencies

# Catch
message(STATUS "Looking for catch")
find_file(FILE_CATCH_CATCH_HPP catch/catch.hpp)
message(STATUS "Looking for catch - ${FILE_CATCH_CATCH_HPP}")

# FMOD
message(STATUS "Looking for FMOD")
find_file(FILE_FMOD_HPP fmod.hpp)
message(STATUS "Looking for FMOD header - ${FILE_FMOD_HPP}")
find_library(LIBRARY_FMOD fmod)
find_library(LIBRARY_FMOD fmod_vc)
message(STATUS "Looking for FMOD library - ${LIBRARY_FMOD}")
message(STATUS "Looking for FMOD library - ${LIBRARY_FMOD_VC}")

# Freetype
message(STATUS "Looking for freetype")
find_file(FILE_FREETYPE2_FT2BUILD_H freetype2/ft2build.h)
message(STATUS "Looking for freetype - ${FILE_FREETYPE2_FT2BUILD_H}")
find_library(LIBRARY_FREETYPE freetype)
find_library(LIBRARY_FREETYPED freetyped)
message(STATUS "Looking for freetype - ${LIBRARY_FREETYPE}")
message(STATUS "Looking for freetype - ${LIBRARY_FREETYPED}")

# Linux
message(STATUS "Looking for Linux")
find_file(FILE_SYS_INOTIFY_H sys/inotify.h)
message(STATUS "Looking for Linux inotify - ${FILE_SYS_INOTIFY_H}")

# OpenGL
message(STATUS "Looking for OpenGL")
find_file(FILE_GL_GL_H GL/gl.h)
find_file(FILE_GL_GLEXT_H GL/glext.h)
message(STATUS "Looking for OpenGL - ${FILE_GL_GL_H}")
message(STATUS "Looking for OpenGL - ${FILE_GL_GLEXT_H}")
find_library(LIBRARY_GL GL)
find_library(LIBRARY_OPENGL32 opengl32)
message(STATUS "Looking for OpenGL - ${LIBRARY_GL}")
message(STATUS "Looking for OpenGL - ${LIBRARY_OPENGL32}")

# POSIX
message(STATUS "Looking for POSIX")
find_file(FILE_POLL_H poll.h)
message(STATUS "Looking for POSIX poll - ${FILE_POLL_H}")
find_file(FILE_PTHREAD_H pthread.h)
message(STATUS "Looking for POSIX pthread - ${FILE_PTHREAD_H}")
find_library(LIBRARY_PTHREAD pthread)
message(STATUS "Looking for POSIX pthread - ${LIBRARY_PTHREAD}")
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

# json
find_file(FILE_NLOHMANN_JSON_HPP nlohmann/json.hpp)
message(STATUS "Looking for json - ${FILE_NLOHMANN_JSON_HPP}")

# zlib
message(STATUS "Looking for z")
find_file(FILE_ZLIB_H zlib.h)
message(STATUS "Looking for z - ${FILE_ZLIB_H}")
find_library(LIBRARY_Z z)
message(STATUS "Looking for z - ${LIBRARY_Z}")
find_library(LIBRARY_ZLIBSTATICD zlibd)
message(STATUS "Looking for z - ${LIBRARY_ZLIBSTATICD}")

# libpng
message(STATUS "Looking for png")
find_file(FILE_PNG_H png.h)
message(STATUS "Looking for png - ${FILE_PNG_H}")
find_file(FILE_LIBPNG16_PNG_H libpng16/png.h)
message(STATUS "Looking for png - ${FILE_LIBPNG16_PNG_H}")
find_library(LIBRARY_PNG png)
message(STATUS "Looking for png - ${LIBRARY_PNG}")
find_library(LIBRARY_LIBPNG16_STATICD libpng16_staticd)
message(STATUS "Looking for png - ${LIBRARY_LIBPNG16_STATICD}")

# general dependencies

add_library(dep_json INTERFACE)
if(FILE_NLOHMANN_JSON_HPP)
	get_filename_component(_directory_nlohmann "${FILE_NLOHMANN_JSON_HPP}" DIRECTORY)
	get_filename_component(_directory ".." ABSOLUTE BASE_DIR "${_directory_nlohmann}")

	target_include_directories(dep_json INTERFACE "${_directory}")
else()
	message(FATAL_ERROR "Needs json nlohmann")
endif()

add_library(dep_png INTERFACE)
if(FILE_PNG_H AND LIBRARY_PNG)
	get_filename_component(_directory "${FILE_PNG_H}" DIRECTORY)

	target_include_directories(dep_png INTERFACE "${_directory}")
	target_link_libraries(dep_png INTERFACE "${LIBRARY_PNG}")
elseif(FILE_LIBPNG16_PNG_H AND LIBRARY_LIBPNG16_STATICD)
	get_filename_component(_directory "${FILE_LIBPNG16_PNG_H}" DIRECTORY)

	target_include_directories(dep_png INTERFACE "${_directory}")
	target_link_libraries(dep_png INTERFACE "${LIBRARY_LIBPNG16_STATICD}")
else()
	message(FATAL_ERROR "Needs png")
endif()

add_library(dep_zlib INTERFACE)
if(FILE_ZLIB_H AND LIBRARY_Z)
	get_filename_component(_directory "${FILE_ZLIB_H}" DIRECTORY)

	target_include_directories(dep_zlib INTERFACE "${_directory}")
	target_link_libraries(dep_zlib INTERFACE "${LIBRARY_Z}")
elseif(FILE_ZLIB_H AND LIBRARY_ZLIBSTATICD)
	get_filename_component(_directory "${FILE_ZLIB_H}" DIRECTORY)

	target_include_directories(dep_zlib INTERFACE "${_directory}")
	target_link_libraries(dep_zlib INTERFACE "${LIBRARY_ZLIBSTATICD}")
else()
	message(FATAL_ERROR "Needs zlib")
endif()

add_library(dep_catch INTERFACE)
if(FILE_CATCH_CATCH_HPP)
	get_filename_component(_directory_catch "${FILE_CATCH_CATCH_HPP}" DIRECTORY)
	get_filename_component(_directory ".." ABSOLUTE BASE_DIR "${_directory_catch}")

	target_include_directories(dep_catch INTERFACE "${_directory}")
else()
	message(WARNING "Missing catch")
endif()

# module dependencies
include(CMakeDependentOption)

# audio
if(FILE_FMOD_HPP AND (LIBRARY_FMOD OR LIBRARY_FMOD_VC))
	set(FOUND_FMOD 1)
endif()
# hack to simplify the expression for `cmake_dependent_option`
cmake_dependent_option(AUDIO_USE_FMOD "Use the FMOD library for the audio module" ON "FOUND_FMOD" OFF)
add_library(audio_dep INTERFACE)
if(AUDIO_USE_FMOD)
	message(STATUS "Audio - FMOD")

	get_filename_component(_directory "${FILE_FMOD_HPP}" DIRECTORY)

	target_include_directories(audio_dep INTERFACE "${_directory}")
	if(LIBRARY_FMOD)
		target_link_libraries(audio_dep INTERFACE "${LIBRARY_FMOD}")
	else()
		target_link_libraries(audio_dep INTERFACE "${LIBRARY_FMOD_VC}")
	endif()
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
cmake_dependent_option(FILE_USE_INOTIFY "Use the Linux inotify API for the file module" ON "FILE_SYS_INOTIFY_H" OFF)
cmake_dependent_option(FILE_USE_KERNEL32 "Use the Windows file API for the file module" ON "FILE_FILEAPI_H AND LIBRARY_KERNEL32" OFF)
_resolve_options(FILE_USE_INOTIFY FILE_USE_KERNEL32)
if(FILE_USE_INOTIFY)
	message(STATUS "File - inotify")
elseif(FILE_USE_KERNEL32)
	message(STATUS "File - Kernel32")
else()
	message(WARNING "File - Disabled")
	set(FILE_USE_DUMMY 1)
endif()

# graphics
if(FILE_GL_GL_H AND FILE_GL_GLEXT_H AND (LIBRARY_GL OR LIBRARY_OPENGL32))
	set(FOUND_OPENGL 1)
endif()
# hack to simplify the expression for `cmake_dependent_option`
cmake_dependent_option(GRAPHICS_USE_OPENGL "Use OpenGL for the graphics module" ON "FOUND_OPENGL" OFF)
add_library(graphics_dep INTERFACE)
if(GRAPHICS_USE_OPENGL)
	message(STATUS "Graphics - OpenGL")

	# note GL.h is a system header, glext.h is not
	get_filename_component(_directory_gl "${FILE_GL_GLEXT_H}" DIRECTORY)
	get_filename_component(_directory ".." ABSOLUTE BASE_DIR "${_directory_gl}")

	target_include_directories(graphics_dep INTERFACE "${_directory}")
	if(LIBRARY_GL)
		target_link_libraries(graphics_dep INTERFACE "${LIBRARY_GL}")
	else()
		target_link_libraries(graphics_dep INTERFACE "${LIBRARY_OPENGL32}")
	endif()
else()
	message(FATAL_ERROR "Graphics - Disabled")
endif()

# window
cmake_dependent_option(WINDOW_USE_USER32 "Use the Windows API for the window module" ON "LIBRARY_GDI32 AND LIBRARY_USER32" OFF)
cmake_dependent_option(WINDOW_USE_X11 "Use X11/Xlib for the window module" ON "FILE_X11_XLIB_H AND LIBRARY_X11" OFF)
_resolve_options(WINDOW_USE_USER32 WINDOW_USE_X11)
add_library(window_dep INTERFACE)
if(WINDOW_USE_USER32)
	message(STATUS "Window - User32")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_USER32}")
elseif(WINDOW_USE_X11)
	message(STATUS "Window - X11")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_X11}")
else()
	message(FATAL_ERROR "Window - Disabled")
endif()

# additional libraries

# input
cmake_dependent_option(INPUT_HAS_USER32_HID "Enable HID in the input module" ON "WINDOW_USE_USER32 AND FILE_HIDSDI_H AND LIBRARY_HID" OFF)
add_library(input_dep INTERFACE)
if(INPUT_HAS_USER32_HID)
	message(STATUS "Input - User32 Hid")

	target_link_libraries(window_dep INTERFACE "${LIBRARY_HID}")
endif()

# text
if(FILE_FREETYPE2_FT2BUILD_H AND (LIBRARY_FREETYPE OR LIBRARY_FREETYPED))
	set(FOUND_FREETYPE 1)
endif()
# hack to simplify the expression for `cmake_dependent_option`
cmake_dependent_option(TEXT_USE_FREETYPE "Use Freetype as text/font source" ON "FOUND_FREETYPE" OFF)
cmake_dependent_option(TEXT_USE_USER32 "Use the Windows API as text/font source" ON "WINDOW_USE_USER32" OFF)
cmake_dependent_option(TEXT_USE_X11 "Use X11/Xlib as text/font source" ON "WINDOW_USE_X11" OFF)
_resolve_options(TEXT_USE_FREETYPE | TEXT_USE_USER32 TEXT_USE_X11)
add_library(text_dep INTERFACE)
if(TEXT_USE_FREETYPE)
	message(STATUS "Text - Freetype")

	get_filename_component(_directory_freetype2 "${FILE_FREETYPE2_FT2BUILD_H}" DIRECTORY)

	target_include_directories(text_dep INTERFACE "${_directory_freetype2}")
	if(LIBRARY_FREETYPE)
		target_link_libraries(text_dep INTERFACE "${LIBRARY_FREETYPE}")
	else()
		target_link_libraries(text_dep INTERFACE "${LIBRARY_FREETYPED}")
	endif()
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
# sse
message(STATUS "Looking for SSE")
find_file(FILE_IMMINTRIN_H immintrin.h)
message(STATUS "Looking for SSE - ${FILE_IMMINTRIN_H}")
if (FILE_IMMINTRIN_H)
  set(HAVE_IMMINTRIN_H 1)
endif()
# if (FILE_XMMINTRIN_H)
#   try_compile(
# 	HAVE_XMMINTRIN_H
# 	"${CMAKE_BINARY_DIR}"
# 	"${CMAKE_SOURCE_DIR}/try/xmmintrin.h.cpp")
#   message(STATUS "Looking for SSE - ${HAVE_XMMINTRIN_H}")
# endif()
# version helpers (Windows only)
if (WINDOW_USE_USER32)
  message(STATUS "Looking for versionhelpers.h")
  find_file(FILE_VERSIONHELPERS_H versionhelpers.h)
  message(STATUS "Looking for versionhelpers.h - ${FILE_VERSIONHELPERS_H}")
  if (FILE_VERSIONHELPERS_H)
    set(HAVE_VERSIONHELPERS_H 1)
  endif()
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
