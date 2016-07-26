
option(ENABLE_TEST "Enable Catch tests" ON)

if(ENABLE_TEST)

	message(STATUS "Catch tests are enabled")
	cmake_minimum_required(VERSION 2.8)
	project(TribunalTest CXX)
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

	# include catch headers
	include_directories("${CMAKE_BINARY_DIR}/catch/src/catch/include")

	# source listing
	set(TRIBUNAL_TEST_SOURCE
		main.cpp
		core/async/delayTest.cpp
		core/crypto/crc.cpp
		core/debug.cpp
		core/maths/Matrix.cpp
		core/maths/Quaternion.cpp
		core/maths/Vector.cpp
		core/maths/util.cpp
		utility/array.cpp
		)

	# source listing
	set(TRIBUNAL_TEST_HEADER
		)

	set(TRIBUNAL_TEST_FILES ${TRIBUNAL_TEST_SOURCE} ${TRIBUNAL_TEST_HEADER})

	foreach(FILE ${TRIBUNAL_TEST_FILES})
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

	add_executable(TribunalTest ${TRIBUNAL_TEST_FILES})

	add_dependencies(TribunalTest Tribunal)
	add_dependencies(TribunalTest catch)

else()
	message(STATUS "Catch tests are disabled")
endif(ENABLE_TEST)
