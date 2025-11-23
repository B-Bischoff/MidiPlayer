cmake_minimum_required(VERSION 3.5)

project(MidiPlayer)

set(CMAKE_CXX_STANDARD 17)

# Build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type)
if (_build_type STREQUAL debug)
	message(STATUS "Debug build")
	add_compile_options(-g3 -O0)
elseif (_build_type STREQUAL release)
	message(STATUS "Release build")
	add_compile_options(-O3)
else()
	message(FATAL_ERROR "Unkown build type: ${CMAKE_BUILD_TYPE}")
endif ()

add_subdirectory(external)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio)
target_link_libraries(${PROJECT_NAME} PRIVATE cereal)
target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
target_link_libraries(${PROJECT_NAME} PRIVATE stb)
target_link_libraries(${PROJECT_NAME} PRIVATE tinysoundfont)

# Remove terminal window (Windows only)
if (WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Deploy resources to build directory
set(RESSOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(DEST_DIR $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources)

add_custom_target(deploy ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESSOURCE_DIR} ${DEST_DIR}
	COMMENT "Deploying resources directory to ${DEST_DIR}"
)
