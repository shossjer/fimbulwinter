
option(ENABLE_TEST "Enable Catch tests" ON)

if(ENABLE_TEST)

	message(STATUS "Catch tests are enabled")
	cmake_minimum_required(VERSION 2.8)
	include(ExternalProject)
	find_package(Git REQUIRED)

	ExternalProject_Add(
		catch
		PREFIX "${CMAKE_BINARY_DIR}/catch"
		GIT_REPOSITORY "https://github.com/philsquared/Catch.git"
		GIT_TAG "master"
		TIMEOUT 10
		UPDATE_COMMAND ""
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ""
		LOG_DOWNLOAD ON
	)

	include (src/CMakeLists.txt)

	set(CODE_TEST_FILES ${CODE_TEST_SOURCE} ${CODE_TEST_HEADER})

	foreach(FILE ${CODE_TEST_FILES})
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

	# include catch headers
	include_directories("${CMAKE_BINARY_DIR}/catch/src/catch/include")
	include_directories("${PROJECT_BINARY_DIR}")
	include_directories("src")
	include_directories("../code/src")
	if (GENERAL_INCLUDES)
		include_directories("${GENERAL_INCLUDES}")
	endif()

	add_executable(CodeTest ${CODE_TEST_FILES})

	add_dependencies(CodeTest catch)
	add_dependencies(CodeTest Code)

	target_link_libraries(CodeTest Code)
	target_link_libraries(CodeTest ${GENERAL_LIBRARIES})
	target_link_libraries(CodeTest ${DEBUG_LIBRARIES})
	target_link_libraries(CodeTest ${OPTIMIZED_LIBRARIES})

else()
	message(STATUS "Catch tests are disabled")
endif(ENABLE_TEST)
