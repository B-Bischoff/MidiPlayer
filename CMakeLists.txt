cmake_minimum_required(VERSION 3.0)

project(MidiPlayer)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(external)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/portmidi/pm_common)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE GLEW)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE cereal)
target_link_libraries(${PROJECT_NAME} PRIVATE GL)
target_link_libraries(${PROJECT_NAME} PRIVATE dl)
