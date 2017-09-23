#
cmake_minimum_required(VERSION 3.6)
project(hellskitchen)

if (MSVC)
	include_directories("C:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/um;C:/gaia/dep/src")
	set(CMAKE_INCLUDE_PATH "C:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/um;C:/gaia/dep/src")
	set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.15063.0/um/x86;C:/gaia/dep/lib")
	set(CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} /wd4100 /wd4127 /wd4189 /wd4512 /wd4702 /MTd /D_SCL_SECURE_NO_WARNINGS"
		CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
    add_definitions(-DNOMINMAX)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HellsKitchen)
endif()

# make debug build default
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(MODE_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
  set(MODE_RELEASE 1)
elseif(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
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

# flags to control physics library linking
set(PHYSICS_USE_BOX2D 0)
set(PHYSICS_USE_PHYSX 0)

# lookup dependencies
if (PHYSICS_USE_BOX2D)
	find_file(FILE_PHYSICS_BOX_HEADERS Box2D/Box2D.h)
	message(STATUS "Looking for Physics Box header files - ${FILE_PHYSICS_BOX_HEADERS}")

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
endif()
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
find_library(LIBRARY_GLEXT GLext)
find_library(LIBRARY_GLEXT32 glext32)
message(STATUS "Looking for OpenGL - ${LIBRARY_GLEXT}")
message(STATUS "Looking for OpenGL - ${LIBRARY_GLEXT32}")
# physics PHYSX
if (PHYSICS_USE_PHYSX)
	find_file(FILE_PHYSX_HEADERS physx/PxPhysicsAPI.h)
	message(STATUS "Looking for Physx header files - ${FILE_PHYSX_HEADERS}")
	# release
	find_library(LIBRARY_PHYSX_CORE PhysX3_x64)
	if (NOT LIBRARY_PHYSX_CORE)
	  find_library(LIBRARY_PHYSX_CORE PhysX3_x86)
	endif()
	find_library(LIBRARY_PHYSX_COMMON PhysX3Common_x64)
	if (NOT LIBRARY_PHYSX_COMMON)
	  find_library(LIBRARY_PHYSX_COMMON PhysX3Common_x86)
	endif()
	find_library(LIBRARY_PHYSX_EXTENSIONS PhysX3Extensions)
	find_library(LIBRARY_PHYSX_PROFILE PhysXProfileSDK)
	find_library(LIBRARY_PHYSX_CHARACTER PhysX3CharacterKinematic_x64)
	if (NOT LIBRARY_PHYSX_CHARACTER)
	  find_library(LIBRARY_PHYSX_CHARACTER PhysX3CharacterKinematic_x86)
	endif()
	find_library(LIBRARY_PHYSX_COOKING PhysX3Cooking_x64)
	if (NOT LIBRARY_PHYSX_COOKING)
	  find_library(LIBRARY_PHYSX_COOKING PhysX3Cooking_x86)
	endif()
	# debug
	find_library(LIBRARY_PHYSX_CORE_D PhysX3DEBUG_x64)
	if (NOT LIBRARY_PHYSX_CORE_D)
	  find_library(LIBRARY_PHYSX_CORE_D PhysX3DEBUG_x86)
	endif()
	find_library(LIBRARY_PHYSX_COMMON_D PhysX3CommonDEBUG_x64)
	if (NOT LIBRARY_PHYSX_COMMON_D)
	  find_library(LIBRARY_PHYSX_COMMON_D PhysX3CommonDEBUG_x86)
	endif()
	find_library(LIBRARY_PHYSX_EXTENSIONS_D PhysX3ExtensionsDEBUG)
	find_library(LIBRARY_PHYSX_PROFILE_D PhysXProfileSDKDEBUG)
	find_library(LIBRARY_PHYSX_CHARACTER_D PhysX3CharacterKinematicDEBUG_x64)
	if (NOT LIBRARY_PHYSX_CHARACTER_D)
	  find_library(LIBRARY_PHYSX_CHARACTER_D PhysX3CharacterKinematicDEBUG_x86)
	endif()
	find_library(LIBRARY_PHYSX_COOKING_D PhysX3CookingDEBUG_x64)
	if (NOT LIBRARY_PHYSX_COOKING_D)
	  find_library(LIBRARY_PHYSX_COOKING_D PhysX3CookingDEBUG_x86)
	endif()

	message(STATUS "Looking for Physx core - ${LIBRARY_PHYSX_CORE}")
	message(STATUS "Looking for Physx commons - ${LIBRARY_PHYSX_COMMON}")
	message(STATUS "Looking for Physx extensions - ${LIBRARY_PHYSX_EXTENSIONS}")
	message(STATUS "Looking for Physx profile - ${LIBRARY_PHYSX_PROFILE}")
	message(STATUS "Looking for Physx character - ${LIBRARY_PHYSX_CHARACTER}")
	message(STATUS "Looking for Physx cooking - ${LIBRARY_PHYSX_COOKING}")

	message(STATUS "Looking for Physx core debug - ${LIBRARY_PHYSX_CORE_D}")
	message(STATUS "Looking for Physx commons debug - ${LIBRARY_PHYSX_COMMON_D}")
	message(STATUS "Looking for Physx extensions debug - ${LIBRARY_PHYSX_EXTENSIONS_D}")
	message(STATUS "Looking for Physx profile debug - ${LIBRARY_PHYSX_PROFILE_D}")
	message(STATUS "Looking for Physx character debug - ${LIBRARY_PHYSX_CHARACTER_D}")
	message(STATUS "Looking for Physx cooking debug - ${LIBRARY_PHYSX_COOKING_D}")
endif()
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

message(STATUS "Looking for libpng16d")
find_library(LIBRARY_LIBPNG libpng16d)
message(STATUS "Looking for libpng16d - ${LIBRARY_LIBPNG}")

set(GENERAL_INCLUDES)

set(GENERAL_LIBRARIES)
set(DEBUG_LIBRARIES)
set(OPTIMIZED_LIBRARIES)

if(LIBRARY_Z)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_Z}")
elseif(LIBRARY_ZLIB)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_ZLIB}")
else()
  message(FATAL_ERROR "Needs zlib")
endif()

if(LIBRARY_PNG)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_PNG}")
elseif(LIBRARY_LIBPNG)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_LIBPNG}")
else()
  message(FATAL_ERROR "Needs png")
endif()

# physics PHYSX
if (PHYSICS_USE_PHYSX)
	get_filename_component(DIRECTORY_OF_PHYSX "${FILE_PHYSX_HEADERS}" DIRECTORY)
	set(GENERAL_INCLUDES "${GENERAL_INCLUDES};${DIRECTORY_OF_PHYSX}")

	# graphics libraries
	if (LIBRARY_PHYSX_CORE)
	  # debug sets lib for "debug" in VS and optimized sets lib for "release" mode in VS
	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_CORE_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_CORE}")

	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_COMMON_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_COMMON}")

	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_EXTENSIONS_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_EXTENSIONS}")

	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_PROFILE_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_PROFILE}")

	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_CHARACTER_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_CHARACTER}")

	  set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSX_COOKING_D}")
	  set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSX_COOKING}")
	endif()
endif()

# physics BOX2D
if (PHYSICS_USE_BOX2D)
	get_filename_component(DIRECTORY_OF_PHYSICS_BOX "${FILE_PHYSICS_BOX_HEADERS}" DIRECTORY)
	set(GENERAL_INCLUDES "${GENERAL_INCLUDES};${DIRECTORY_OF_PHYSICS_BOX}")

	set(DEBUG_LIBRARIES "${DEBUG_LIBRARIES};${LIBRARY_PHYSICS_BOX_CORE_D}")
	set(OPTIMIZED_LIBRARIES "${OPTIMIZED_LIBRARIES};${LIBRARY_PHYSICS_BOX_CORE}")
endif()

# select libraries
# graphics
if (FILE_GL_GL_H AND
	LIBRARY_GL)
  message(STATUS "Graphics - OpenGL")
  set(GRAPHICS_USE_OPENGL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_GL}")
elseif (FILE_GL_GL_H AND
	LIBRARY_OPENGL32)
  message(STATUS "Graphics - OpenGL")
  set(GRAPHICS_USE_OPENGL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_OPENGL32}")
else()
  message(FATAL_ERROR "Graphics - Disabled")
endif()

# sound
if (FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL)
  message(STATUS "Sound - OpenAL")
  set(SOUND_USE_OPENAL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_OPENAL}")
elseif(FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL32)
  message(STATUS "Sound - OpenAL")
  set(SOUND_USE_OPENAL 1)
  set(GENERAL_LIBRARIES "${GENERAL_LIBRARIES};${LIBRARY_OPENAL32}")
else()
  message(WARNING "Sound - Disabled")
endif()

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

add_subdirectory(code)
add_subdirectory(game)

enable_testing(false)
add_subdirectory(tst)

# write config
configure_file(
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")
