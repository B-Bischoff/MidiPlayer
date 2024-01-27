cmake_minimum_required(VERSION 3.0)

project(MidiPlayer)

find_package(ALSA REQUIRED)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(external/portaudio)
add_subdirectory(external/portmidi)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/portmidi/pm_common)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE asound)
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE PortAudio)
target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
