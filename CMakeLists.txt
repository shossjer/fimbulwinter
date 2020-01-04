
cmake_minimum_required(VERSION 3.5.1)

project(hellskitchen C CXX)

# https://stackoverflow.com/a/31662300
if(CMAKE_CONFIGURATION_TYPES)
	unset(CMAKE_BUILD_TYPE CACHE)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
elseif(NOT CMAKE_BUILD_TYPE)
	message(FATAL_ERROR "Single configurations need to specify CMAKE_BUILD_TYPE! Run cmake with something like:\n-DCMAKE_BUILD_TYPE=Debug|Release")
endif()
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug)

include_directories("${PROJECT_SOURCE_DIR}/dep/include")
set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH};${PROJECT_SOURCE_DIR}/dep/include")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${PROJECT_SOURCE_DIR}/dep/lib")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if (MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG
	 "${CMAKE_CXX_FLAGS_DEBUG} /wd4100 /wd4127 /wd4189 /wd4512 /wd4702 /MTd /D_SCL_SECURE_NO_WARNINGS"
	 CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  add_definitions(-DNOMINMAX)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT hellskitchen)
endif ()

include(settings.cmake)

if (CUSTOM_INCLUDE)
	include_directories(${CUSTOM_INCLUDE})
	set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH};${CUSTOM_INCLUDE}")
endif()
if (CUSTOM_LIBRARY)
	set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CUSTOM_LIBRARY}")
endif()

# project information
message(STATUS "Using compiler ${CMAKE_CXX_COMPILER}")

message(STATUS "CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
message(STATUS "PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")

message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_SYSTEM_PREFIX_PATH=${CMAKE_SYSTEM_PREFIX_PATH}")
message(STATUS "CMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}")
message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")

# hardware settings
include(TestBigEndian)
test_big_endian(ENDIANESS)
if (ENDIANESS)
  set(BIG_WORDS 1)
endif ()

# lookup dependencies
# Catch
message(STATUS "Looking for catch")
find_file(FILE_CATCH_HPP catch.hpp)
find_file(FILE_CATCH_CATCH_HPP catch/catch.hpp)
message(STATUS "Looking for catch - ${FILE_CATCH_HPP}")
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
find_file(FILE_FT2BUILD_H ft2build.h)
find_file(FILE_FREETYPE2_FT2BUILD_H freetype2/ft2build.h)
message(STATUS "Looking for freetype - ${FILE_FT2BUILD_H}")
message(STATUS "Looking for freetype - ${FILE_FREETYPE2_FT2BUILD_H}")
find_library(LIBRARY_FREETYPE freetype)
find_library(LIBRARY_FREETYPED freetyped)
message(STATUS "Looking for freetype - ${LIBRARY_FREETYPE}")
message(STATUS "Looking for freetype - ${LIBRARY_FREETYPED}")
# Gdi32
message(STATUS "Looking for Gdi32")
find_library(LIBRARY_GDI32 gdi32)
message(STATUS "Looking for Gdi32 - ${LIBRARY_GDI32}")
# Hid
message(STATUS "Looking for Hid")
find_file(FILE_HIDSDI_H Hidsdi.h)
message(STATUS "Looking for Hid - ${FILE_HIDSDI_H}")
find_library(LIBRARY_HID Hid)
message(STATUS "Looking for Hid - ${LIBRARY_HID}")
# Kernel32
message(STATUS "Looking for Kernel32")
find_library(LIBRARY_KERNEL32 kernel32)
message(STATUS "Looking for Kernel32 - ${LIBRARY_KERNEL32}")
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
# User32
message(STATUS "Looking for User32")
find_library(LIBRARY_USER32 user32)
message(STATUS "Looking for User32 - ${LIBRARY_USER32}")
# X11
message(STATUS "Looking for X11")
find_file(FILE_X11_XLIB_H X11/Xlib.h)
message(STATUS "Looking for X11 - ${FILE_X11_XLIB_H}")
find_library(LIBRARY_X11 X11)
message(STATUS "Looking for X11 - ${LIBRARY_X11}")

# look for JSON library hpp file
find_file(FILE_JSON_HEADER nlohmann/json.hpp)
message(STATUS "Looking for JSON header file - ${FILE_JSON_HEADER}")
if (FILE_JSON_HEADER)
else()
  message(FATAL_ERROR "JSON parser - You need to get one!")
endif()

# zlib
message(STATUS "Looking for z")
find_library(LIBRARY_Z z)
message(STATUS "Looking for z - ${LIBRARY_Z}")

message(STATUS "Looking for zlibd")
find_library(LIBRARY_ZLIB zlibd)
message(STATUS "Looking for zlibd - ${LIBRARY_ZLIB}")

# libpng
message(STATUS "Looking for png")
find_library(LIBRARY_PNG png)
message(STATUS "Looking for png - ${LIBRARY_PNG}")

message(STATUS "Looking for libpng16_staticd")
find_library(LIBRARY_LIBPNG libpng16_staticd)
message(STATUS "Looking for libpng16_staticd - ${LIBRARY_LIBPNG}")

set(GENERAL_INCLUDES)

set(GENERAL_LIBRARIES)
set(DEBUG_LIBRARIES)
set(OPTIMIZED_LIBRARIES)

if(LIBRARY_PNG)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_PNG}")
elseif(LIBRARY_LIBPNG)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_LIBPNG}")
else()
  message(FATAL_ERROR "Needs png")
endif()

if(LIBRARY_Z)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_Z}")
elseif(LIBRARY_ZLIB)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_ZLIB}")
else()
  message(FATAL_ERROR "Needs zlib")
endif()

# select libraries
# audio
if(FILE_FMOD_HPP AND (LIBRARY_FMOD OR LIBRARY_FMOD_VC))
	message(STATUS "Audio - FMOD")
	set(AUDIO_USE_FMOD 1)
	if(LIBRARY_FMOD)
		set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_FMOD}")
	else()
		set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_FMOD_VC}")
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

# graphics
if (FILE_GL_GL_H AND
	 FILE_GL_GLEXT_H AND
	 LIBRARY_GL)
  message(STATUS "Graphics - OpenGL")
  set(GRAPHICS_USE_OPENGL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_GL}")
elseif (FILE_GL_GL_H AND
	 FILE_GL_GLEXT_H AND
	 LIBRARY_OPENGL32)
  message(STATUS "Graphics - OpenGL")
  set(GRAPHICS_USE_OPENGL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_OPENGL32}")
else()
  message(FATAL_ERROR "Graphics - Disabled")
endif()

# test
if (FILE_CATCH_HPP OR
	 FILE_CATCH_CATCH_HPP)
  message(STATUS "Test - Catch")

  if (FILE_CATCH_CATCH_HPP)
	 get_filename_component(INCLUDE_DIRECTORY_CATCH "${FILE_CATCH_CATCH_HPP}" DIRECTORY)
	 set(GENERAL_INCLUDES "${GENERAL_INCLUDES};${INCLUDE_DIRECTORY_CATCH}")
  endif()
else ()
  message(WARNING "Test - Disabled")
endif ()

# thread
if (LIBRARY_KERNEL32)
  message(STATUS "Thread - Kernel32")
  set(THREAD_USE_KERNEL32 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_KERNEL32}")
elseif(FILE_PTHREAD_H AND
	LIBRARY_PTHREAD)
  message(STATUS "Thread - Pthread")
  set(THREAD_USE_PTHREAD 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_PTHREAD}")
else()
  message(FATAL_ERROR "Thread - Disabled")
endif()

# window
if (LIBRARY_GDI32 AND
	LIBRARY_USER32)
  message(STATUS "Window - User32")
  set(WINDOW_USE_USER32 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_USER32}")
elseif(FILE_X11_XLIB_H AND
	LIBRARY_X11)
  message(STATUS "Window - X11")
  set(WINDOW_USE_X11 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_X11}")
else()
  message(FATAL_ERROR "Window - Disabled")
endif()

# additional libraries
# input
if(WINDOW_USE_USER32)
	if(FILE_HIDSDI_H AND LIBRARY_HID)
		message(STATUS "Input - User32 Hid")
		set(INPUT_HAS_USER32_HID 1)
		set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_HID}")
	endif()
endif()

# text
if ((FILE_FT2BUILD_H OR
	  FILE_FREETYPE2_FT2BUILD_H) AND
	(LIBRARY_FREETYPE OR LIBRARY_FREETYPED))
  message(STATUS "Text - Freetype")
  set(TEXT_USE_FREETYPE 1)

  if (FILE_FREETYPE2_FT2BUILD_H)
	get_filename_component(INCLUDE_DIRECTORY_FREETYPE "${FILE_FREETYPE2_FT2BUILD_H}" DIRECTORY)
	set(GENERAL_INCLUDES "${GENERAL_INCLUDES};${INCLUDE_DIRECTORY_FREETYPE}")
  endif()

  if (LIBRARY_FREETYPE)
	set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_FREETYPE}")
  else()
	set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_FREETYPED}")
  endif()

elseif (WINDOW_USE_USER32)
  message(WARNING "Text - User32")
  set(TEXT_USE_USER32 1)
elseif (WINDOW_USE_X11)
  message(WARNING "Text - X11")
  set(TEXT_USE_X11 1)
else()
  message(WARNING "Text - Disabled")
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
add_subdirectory(game)
