
# source listing
set(SOURCE_FILES
	src/android_native_app_glue.c
	src/main.cpp
	)

set(HEADER_FILES
	src/android_native_app_glue.h
	src/settings.hpp
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
if (GENERAL_INCLUDES)
	include_directories("${GENERAL_INCLUDES}")
endif()

if(WINDOW_USE_ANDROID)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
  add_library(HellsKitchen SHARED ${ALL_FILES})
else()
  add_executable(HellsKitchen ${ALL_FILES})
endif()

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(HellsKitchen PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

add_dependencies(HellsKitchen Code)

target_link_libraries(HellsKitchen Code)
target_link_libraries(HellsKitchen ${GENERAL_LIBRARIES})
target_link_libraries(HellsKitchen ${DEBUG_LIBRARIES})
target_link_libraries(HellsKitchen ${OPTIMIZED_LIBRARIES})
