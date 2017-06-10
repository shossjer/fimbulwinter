#
cmake_minimum_required(VERSION 2.8)
project(hellskitchen)

if (MSVC)
	include_directories("C:/Program Files (x86)/Windows Kits/8.1/Include/um")
	set(CMAKE_INCLUDE_PATH "C:/Program Files (x86)/Windows Kits/8.1/Include/um")
	set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x86")
	set(CMAKE_CXX_FLAGS_DEBUG
		"${CMAKE_CXX_FLAGS_DEBUG} /wd4100 /wd4127 /wd4189 /wd4512 /wd4702 /MTd /D_SCL_SECURE_NO_WARNINGS"
		CACHE STRING "Debug builds CMAKE CXX flags " FORCE )
endif()

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

# flags to control physics library linking
set(PHYSICS_USE_BOX2D 0)
set(PHYSICS_USE_PHYSX 0)

include (src/CMakeLists.txt)

set(ALL_FILES ${SOURCE_FILES} ${HEADER_FILES})

foreach(FILE ${ALL_FILES})
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

add_executable(HellsKitchen ${ALL_FILES})

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
find_library(LIBRARY_GLEXT GLext)
find_library(LIBRARY_GLEXT32 glext32)
message(STATUS "Looking for OpenGL - ${LIBRARY_GLEXT}")
message(STATUS "Looking for OpenGL - ${LIBRARY_GLEXT32}")
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

message(STATUS "Looking for zdll")
find_library(LIBRARY_ZDLL zdll)
message(STATUS "Looking for zdll - ${LIBRARY_ZDLL}")

if(LIBRARY_Z)
  target_link_libraries(HellsKitchen ${LIBRARY_Z})
elseif(LIBRARY_ZDLL)
  target_link_libraries(HellsKitchen ${LIBRARY_ZDLL})
else()
  message(FATAL_ERROR "Needs zlib")
endif()

# libpng
message(STATUS "Looking for png")
find_library(LIBRARY_PNG png)
message(STATUS "Looking for png - ${LIBRARY_PNG}")

if(LIBRARY_PNG)
  target_link_libraries(HellsKitchen ${LIBRARY_PNG})
else()
  message(FATAL_ERROR "Needs png")
endif()

# physics PHYSX
#
# Check that Physx header files exist
if (PHYSICS_USE_PHYSX)
	find_file(FILE_PHYSX_HEADERS physx/PxPhysicsAPI.h)
	get_filename_component(DIRECTORY_OF_PHYSX "${FILE_PHYSX_HEADERS}" DIRECTORY)
	include_directories("${DIRECTORY_OF_PHYSX}")

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

	# graphics libraries
	if (LIBRARY_PHYSX_CORE)
	  # debug sets lib for "debug" in VS and optimized sets lib for "release" mode in VS
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_CORE_D} optimized ${LIBRARY_PHYSX_CORE})
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_COMMON_D} optimized ${LIBRARY_PHYSX_COMMON})
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_EXTENSIONS_D} optimized ${LIBRARY_PHYSX_EXTENSIONS})
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_PROFILE_D} optimized ${LIBRARY_PHYSX_PROFILE})
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_CHARACTER_D} optimized ${LIBRARY_PHYSX_CHARACTER})
	  target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSX_COOKING_D} optimized ${LIBRARY_PHYSX_COOKING})
	endif()
endif()

# physics BOX2D
#
# Check that BOX2d header files exist
if (PHYSICS_USE_BOX2D)
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

	target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSICS_BOX_CORE_D} optimized ${LIBRARY_PHYSICS_BOX_CORE})
endif()


# select libraries
# graphics
if (FILE_GL_GL_H AND
	LIBRARY_GL)
  message(STATUS "Graphics - OpenGL")
  target_link_libraries(HellsKitchen ${LIBRARY_GL})
  set(GRAPHICS_USE_OPENGL 1)
elseif (FILE_GL_GL_H AND
	LIBRARY_OPENGL32)
  message(STATUS "Graphics - OpenGL")
  target_link_libraries(HellsKitchen ${LIBRARY_OPENGL32})
  set(GRAPHICS_USE_OPENGL 1)
else()
  message(FATAL_ERROR "Graphics - Disabled")
endif()

# sound
if (FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(HellsKitchen ${LIBRARY_OPENAL})
  set(SOUND_USE_OPENAL 1)
elseif(FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL32)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(HellsKitchen ${LIBRARY_OPENAL32})
  set(SOUND_USE_OPENAL 1)
else()
  message(WARNING "Sound - Disabled")
endif()

# thread
if (LIBRARY_KERNEL32)
  message(STATUS "Thread - Kernel32")
  target_link_libraries(HellsKitchen ${LIBRARY_KERNEL32})
  set(THREAD_USE_KERNEL32 1)
elseif(FILE_PTHREAD_H AND
	LIBRARY_PTHREAD)
  message(STATUS "Thread - Pthread")
  target_link_libraries(HellsKitchen ${LIBRARY_PTHREAD})
  set(THREAD_USE_PTHREAD 1)
else()
  message(FATAL_ERROR "Thread - Disabled")
endif()

# window
if (LIBRARY_GDI32 AND
	LIBRARY_USER32)
  message(STATUS "Window - User32")
  target_link_libraries(HellsKitchen ${LIBRARY_USER32})
  set(WINDOW_USE_USER32 1)
elseif(FILE_X11_XLIB_H AND
	LIBRARY_X11)
  message(STATUS "Window - X11")
  target_link_libraries(HellsKitchen ${LIBRARY_X11})
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
