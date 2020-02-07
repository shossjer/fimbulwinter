
set(SOURCES_HELLSKITCHEN
	src/main.cpp
	)

set(HEADERS_HELLSKITCHEN
	src/settings.hpp
	)

add_executable(hellskitchen "")
target_sources(hellskitchen PRIVATE ${SOURCES_HELLSKITCHEN} ${HEADERS_HELLSKITCHEN})
target_include_directories(hellskitchen PRIVATE "src" "${GENERAL_INCLUDES}")
target_link_libraries(hellskitchen PRIVATE generated utility core engine gameplay ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})
target_compile_options(hellskitchen PRIVATE ${private_compile_options})
target_compile_definitions(hellskitchen PRIVATE ${private_compile_definitions})

# set_target_properties(hellskitchen PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
#
# is what we would like to do, however, set_target_properties does not support
# generator expressions. we therefore have to do this ugly thing instead
if(MSVC)
	set_target_properties(hellskitchen PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
endif()

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(hellskitchen PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()
