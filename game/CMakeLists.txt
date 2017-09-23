
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

target_link_libraries(HellsKitchen ${GENERAL_LIBRARIES})
target_link_libraries(HellsKitchen ${DEBUG_LIBRARIES})
target_link_libraries(HellsKitchen ${OPTIMIZED_LIBRARIES})
