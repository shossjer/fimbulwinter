
# source listing
set(SOURCE_FILES
	src/main.cpp
	)

set(HEADER_FILES
	)

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
include_directories("src")
include_directories("../code/src")
if (PHYSICS_USE_PHYSX)
	get_filename_component(DIRECTORY_OF_PHYSX "${FILE_PHYSX_HEADERS}" DIRECTORY)
	include_directories("${DIRECTORY_OF_PHYSX}")
endif()
if (PHYSICS_USE_BOX2D)
	get_filename_component(DIRECTORY_OF_PHYSICS_BOX "${FILE_PHYSICS_BOX_HEADERS}" DIRECTORY)
	include_directories("${DIRECTORY_OF_PHYSICS_BOX}")
endif()
if ((FILE_FT2BUILD_H OR
	  FILE_FREETYPE2_FT2BUILD_H) AND
	(LIBRARY_FREETYPE OR LIBRARY_FREETYPED))
	if (FILE_FREETYPE2_FT2BUILD_H)
		get_filename_component(INCLUDE_DIRECTORY_FREETYPE "${FILE_FREETYPE2_FT2BUILD_H}" DIRECTORY)
		include_directories("${INCLUDE_DIRECTORY_FREETYPE}")
	endif()
endif()

add_executable(HellsKitchen ${ALL_FILES})

add_dependencies(HellsKitchen Code)

target_link_libraries(HellsKitchen Code)

if(LIBRARY_Z)
  target_link_libraries(HellsKitchen ${LIBRARY_Z})
elseif(LIBRARY_ZLIB)
  target_link_libraries(HellsKitchen ${LIBRARY_ZLIB})
else()
  message(FATAL_ERROR "Needs zlib")
endif()

if(LIBRARY_PNG)
  target_link_libraries(HellsKitchen ${LIBRARY_PNG})
elseif(LIBRARY_LIBPNG)
  target_link_libraries(HellsKitchen ${LIBRARY_LIBPNG})
else()
  message(FATAL_ERROR "Needs png")
endif()

if (PHYSICS_USE_PHYSX)
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

if (PHYSICS_USE_BOX2D)
	target_link_libraries(HellsKitchen debug ${LIBRARY_PHYSICS_BOX_CORE_D} optimized ${LIBRARY_PHYSICS_BOX_CORE})
endif()

# select libraries graphics
if (FILE_GL_GL_H AND LIBRARY_GL)
  target_link_libraries(HellsKitchen ${LIBRARY_GL})
elseif (FILE_GL_GL_H AND LIBRARY_OPENGL32)
  target_link_libraries(HellsKitchen ${LIBRARY_OPENGL32})
else()
  message(FATAL_ERROR "Graphics - Disabled")
endif()

# sound
if (FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(HellsKitchen ${LIBRARY_OPENAL})
elseif(FILE_AL_AL_H AND
	FILE_AL_ALC_H AND
	LIBRARY_OPENAL32)
  message(STATUS "Sound - OpenAL")
  target_link_libraries(HellsKitchen ${LIBRARY_OPENAL32})
else()
  message(WARNING "Sound - Disabled")
endif()

# thread
if (LIBRARY_KERNEL32)
  message(STATUS "Thread - Kernel32")
  target_link_libraries(HellsKitchen ${LIBRARY_KERNEL32})
elseif(FILE_PTHREAD_H AND
	LIBRARY_PTHREAD)
  message(STATUS "Thread - Pthread")
  target_link_libraries(HellsKitchen ${LIBRARY_PTHREAD})
else()
  message(FATAL_ERROR "Thread - Disabled")
endif()

# window
if (LIBRARY_GDI32 AND
	LIBRARY_USER32)
  message(STATUS "Window - User32")
  target_link_libraries(HellsKitchen ${LIBRARY_USER32})
elseif(FILE_X11_XLIB_H AND
	LIBRARY_X11)
  message(STATUS "Window - X11")
  target_link_libraries(HellsKitchen ${LIBRARY_X11})
else()
  message(FATAL_ERROR "Window - Disabled")
endif()

# additional libraries
# text
if ((FILE_FT2BUILD_H OR
	  FILE_FREETYPE2_FT2BUILD_H) AND
	(LIBRARY_FREETYPE OR LIBRARY_FREETYPED))
  message(STATUS "Text - Freetype")

  if (LIBRARY_FREETYPE)
    target_link_libraries(HellsKitchen ${LIBRARY_FREETYPE})
  else()
    target_link_libraries(HellsKitchen ${LIBRARY_FREETYPED})
  endif()
elseif (WINDOW_USE_USER32)
  message(WARNING "Text - User32")
elseif (WINDOW_USE_X11)
  message(WARNING "Text - X11")
else()
  message(WARNING "Text - Disabled")
endif()
