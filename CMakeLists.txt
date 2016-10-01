#
cmake_minimum_required(VERSION 2.8)
project(tribunal)

# make debug build default
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(MODE_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
  set(MODE_RELEASE 1)
else()
  message(FATAL_ERROR "Build type ${CMAKE_BUILD_TYPE} not supported")
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

# the version number
set(TRIBUNAL_VERSION_MAJOR 0)
set(TRIBUNAL_VERSION_MINOR 1)

# source listing
set(TRIBUNAL_SOURCE
	src/main.cpp
	src/core/async/Thread.cpp
	src/core/crypto/crc.cpp
	src/engine/animation/Armature.cpp
	src/engine/animation/mixer.cpp
	src/engine/application/window.cpp
	src/engine/graphics/opengl/Font.cpp
	src/engine/graphics/renderer.cpp
	src/engine/graphics/viewer.cpp
	src/engine/hid/input.cpp
	src/engine/hid/input_user32.cpp
	src/engine/hid/input_x11.cpp
	src/engine/physics/effects.cpp
	src/engine/physics/physics.cpp
	src/engine/physics/queries.cpp
	src/gameplay/characters.cpp
	src/gameplay/effects.cpp
	src/gameplay/level.cpp
	src/gameplay/looper.cpp
	src/gameplay/ui.cpp
	)

# source listing
set(TRIBUNAL_HEADER
	src/core/async/delay.hpp
	src/core/async/Thread.hpp
	src/core/color.hpp
	src/core/container/Buffer.hpp
	src/core/container/CircleQueue.hpp
	src/core/container/Collection.hpp
	src/core/container/ExchangeQueue.hpp
	src/core/container/Stack.hpp
	src/core/crypto/crc.hpp
	src/core/debug.hpp
	src/core/format.hpp
	src/core/maths/Matrix.hpp
	src/core/maths/Plane.hpp
	src/core/maths/Quaternion.hpp
	src/core/maths/Scalar.hpp
	src/core/maths/Vector.hpp
	src/core/maths/algorithm.hpp
	src/core/maths/util.hpp
	src/core/sync/CriticalSection.hpp
	src/core/sync/Mutex.hpp
	src/engine/Entity.hpp
	src/engine/animation/Armature.hpp
	src/engine/animation/mixer.hpp
	src/engine/application/window.hpp
	src/engine/debug.hpp
	src/engine/extras/Asset.hpp
	src/engine/graphics/renderer.hpp
	src/engine/graphics/opengl/Font.hpp
	src/engine/graphics/opengl/Matrix.hpp
	src/engine/graphics/viewer.hpp
	src/engine/hid/input.hpp
	src/engine/physics/defines.hpp
	src/engine/physics/effects.hpp
	src/engine/physics/helper.hpp
	src/engine/physics/physics.hpp
	src/engine/physics/queries.hpp
	src/engine/physics/Callbacks.hpp
	src/gameplay/characters.hpp
	src/gameplay/CharacterState.hpp
	src/gameplay/debug.hpp
	src/gameplay/Effect.hpp
	src/gameplay/effects.hpp
	src/gameplay/level.hpp
	src/gameplay/ui.hpp
	src/utility/algorithm.hpp
	src/utility/aligned_array.hpp
	src/utility/array.hpp
	src/utility/inaccessible.hpp
	src/utility/optional.hpp
	src/utility/stream.hpp
	src/utility/string.hpp
	src/utility/type_traits.hpp
	)

set(TRIBUNAL_FILES ${TRIBUNAL_SOURCE} ${TRIBUNAL_HEADER})

foreach(FILE ${TRIBUNAL_FILES})
	get_filename_component(PARENT_DIR "${FILE}" PATH)

	# skip src or include and changes /'s to \\'s
	string(REGEX REPLACE "(\\./)?(src|include)/?" "" GROUP "${PARENT_DIR}")
	string(REPLACE "/" "\\" GROUP "${GROUP}")

	# group into "Source Files" and "Header Files"
	if ("${FILE}" MATCHES ".*\\.cpp")
		set(GROUP "Source Files\\${GROUP}")
	elseif("${FILE}" MATCHES ".*\\.h")
		set(GROUP "Header Files\\${GROUP}")
	else()
		set(GROUP "Other Files\\${GROUP}")
	endif()

	source_group("${GROUP}" FILES "${FILE}")
endforeach()

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(Tribunal ${TRIBUNAL_FILES})

enable_testing (true)
add_subdirectory (tst)

# lookup dependencies
# User32
message(STATUS "Looking for Gdi32")
find_library(LIBRARY_GDI32 gdi32)
message(STATUS "Looking for Gdi32 - ${LIBRARY_GDI32}")
# Kernel32
message(STATUS "Looking for Kernel32")
find_library(LIBRARY_KERNEL32 kernel32)
message(STATUS "Looking for Kernel32 - ${LIBRARY_KERNEL32}")
# OpenAL
message(STATUS "Looking for OpenAL")
find_file(FILE_AL_AL_H AL/al.h)
find_file(FILE_AL_ALC_H AL/alc.h)
message(STATUS "Looking for OpenAL - ${FILE_AL_AL_H}")
message(STATUS "Looking for OpenAL - ${FILE_AL_ALC_H}")
find_library(LIBRARY_OPENAL openal)
find_library(LIBRARY_OPENAL32 openal32)
message(STATUS "Looking for OpenAL - ${LIBRARY_OPENAL}")
message(STATUS "Looking for OpenAL - ${LIBRARY_OPENAL32}")
# OpenGL
message(STATUS "Looking for OpenGL")
find_file(FILE_GL_GL_H GL/gl.h)
find_file(FILE_GL_GLU_H GL/glu.h)
message(STATUS "Looking for OpenGL - ${FILE_GL_GL_H}")
message(STATUS "Looking for OpenGL - ${FILE_GL_GLU_H}")
find_library(LIBRARY_GL GL)
find_library(LIBRARY_OPENGL32 opengl32)
message(STATUS "Looking for OpenGL - ${LIBRARY_GL}")
message(STATUS "Looking for OpenGL - ${LIBRARY_OPENGL32}")
# Pthread
message(STATUS "Looking for Pthread")
find_file(FILE_PTHREAD_H pthread.h)
message(STATUS "Looking for Pthread - ${FILE_PTHREAD_H}")
find_library(LIBRARY_PTHREAD pthread)
message(STATUS "Looking for Pthread - ${LIBRARY_PTHREAD}")
# User32
message(STATUS "Looking for User32")
find_library(LIBRARY_USER32 user32)
message(STATUS "Looking for User32 - ${LIBRARY_USER32}")
# Pthread
message(STATUS "Looking for X11")
find_file(FILE_X11_XLIB_H X11/Xlib.h)
message(STATUS "Looking for X11 - ${FILE_X11_XLIB_H}")
find_library(LIBRARY_X11 X11)
message(STATUS "Looking for X11 - ${LIBRARY_X11}")

# Check that BOX2d header files exist
find_file(FILE_PHYSICS_BOX_HEADERS Box2D/Box2D.h)
message(STATUS "Looking for Physics Box header files - ${FILE_PHYSICS_BOX_HEADERS}")

get_filename_component(DIRECTORY_OF_PHYSICS_BOX "${FILE_PHYSICS_BOX_HEADERS}" DIRECTORY)
include_directories("${DIRECTORY_OF_PHYSICS_BOX}")
if (MODE_DEBUG)
  add_definitions(-D_DEBUG)
elseif (MODE_RELEASE)
  add_definitions(-DNDEBUG)
endif()

find_library(LIBRARY_PHYSICS_BOX_CORE_D Box2D)
if (NOT LIBRARY_PHYSICS_BOX_CORE_D)
  find_library(LIBRARY_PHYSICS_BOX_CORE_D Box2D)
endif()

find_library(LIBRARY_PHYSICS_BOX_CORE Box2D)
if (NOT LIBRARY_PHYSICS_BOX_CORE)
  find_library(LIBRARY_PHYSICS_BOX_CORE Box2D)
endif()

message(STATUS "Looking for Physics Box core - ${LIBRARY_PHYSICS_BOX_CORE}")

target_link_libraries(Tribunal debug ${LIBRARY_PHYSICS_BOX_CORE_D} optimized ${LIBRARY_PHYSICS_BOX_CORE})

# select libraries
# graphics
if (FILE_GL_GL_H AND
	LIBRARY_GL)
  message(STATUS "Graphics - OpenGL")
  target_link_libraries(Tribunal ${LIBRARY_GL})
  set(GRAPHICS_USE_OPENGL 1)
elseif (FILE_GL_GL_H AND
	LIBRARY_OPENGL32)
  message(STATUS "Graphics - OpenGL")
  target_link_libraries(Tribunal ${LIBRARY_OPENGL32})
  set(GRAPHICS_USE_OPENGL 1)
else()
  message(FATAL_ERROR "Graphics - Disabled")
endif()

# sound
if (FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(Tribunal ${LIBRARY_OPENAL})
  set(SOUND_USE_OPENAL 1)
elseif(FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL32)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(Tribunal ${LIBRARY_OPENAL32})
  set(SOUND_USE_OPENAL 1)
else()
  message(WARNING "Sound - Disabled")
endif()

# thread
if (LIBRARY_KERNEL32)
  message(STATUS "Thread - Kernel32")
  target_link_libraries(Tribunal ${LIBRARY_KERNEL32})
  set(THREAD_USE_KERNEL32 1)
elseif(FILE_PTHREAD_H AND
	LIBRARY_PTHREAD)
  message(STATUS "Thread - Pthread")
  target_link_libraries(Tribunal ${LIBRARY_PTHREAD})
  set(THREAD_USE_PTHREAD 1)
else()
  message(FATAL_ERROR "Thread - Disabled")
endif()

# window
if (LIBRARY_GDI32 AND
	LIBRARY_USER32)
  message(STATUS "Window - User32")
  target_link_libraries(Tribunal ${LIBRARY_USER32})
  set(WINDOW_USE_USER32 1)
elseif(FILE_X11_XLIB_H AND
	LIBRARY_X11)
  message(STATUS "Window - X11")
  target_link_libraries(Tribunal ${LIBRARY_X11})
  set(WINDOW_USE_X11 1)
else()
  message(FATAL_ERROR "Window - Disabled")
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

# write config
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")
