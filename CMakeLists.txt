cmake_minimum_required(VERSION 3.0)

project(MidiPlayer)

find_package(ALSA REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE asound)
