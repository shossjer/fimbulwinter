option(RUN_TESTS "Run tests as part of the build process." ON)

include (src/CMakeLists.txt)

add_executable(utilitytest "")
target_sources(utilitytest PRIVATE ${FILES_UTILITY})
target_include_directories(utilitytest PRIVATE "src" "${GENERAL_INCLUDES}")
target_link_libraries(utilitytest PRIVATE generated utility ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(utilitytest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	add_custom_command(
		TARGET utilitytest
		POST_BUILD
		COMMAND $<TARGET_FILE:utilitytest>
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		)
endif()

add_executable(coretest "")
target_sources(coretest PRIVATE ${FILES_CORE})
target_include_directories(coretest PRIVATE "src" "${GENERAL_INCLUDES}")
target_link_libraries(coretest PRIVATE generated utility core ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(coretest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	add_custom_command(
		TARGET coretest
		POST_BUILD
		COMMAND $<TARGET_FILE:coretest>
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		)
endif()

add_executable(enginetest "")
target_sources(enginetest PRIVATE ${FILES_ENGINE})
target_include_directories(enginetest PRIVATE "src" "${GENERAL_INCLUDES}")
target_link_libraries(enginetest PRIVATE generated utility core engine ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})

if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(enginetest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	add_custom_command(
		TARGET enginetest
		POST_BUILD
		COMMAND $<TARGET_FILE:enginetest>
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		)
endif()

add_executable(gameplaytest "")
target_sources(gameplaytest PRIVATE ${FILES_GAMEPLAY})
target_include_directories(gameplaytest PRIVATE "src" "${GENERAL_INCLUDES}")
target_link_libraries(gameplaytest PRIVATE generated utility core engine gameplay ${GENERAL_LIBRARIES} ${DEBUG_LIBRARIES} ${OPTIMIZED_LIBRARIES})

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.7)
	set_target_properties(gameplaytest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()

if(RUN_TESTS)
	add_custom_command(
		TARGET gameplaytest
		POST_BUILD
		COMMAND $<TARGET_FILE:gameplaytest>
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		)
endif()
