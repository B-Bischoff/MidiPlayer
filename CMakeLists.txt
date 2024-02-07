cmake_minimum_required(VERSION 3.0)

project(MidiPlayer)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(external/portmidi)
add_subdirectory(external/rtaudio)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/portmidi/pm_common)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio)
