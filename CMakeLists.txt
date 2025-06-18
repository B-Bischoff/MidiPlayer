cmake_minimum_required(VERSION 3.5)

project(MidiPlayer)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME}
	${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE portmidi)
target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE implot)
target_link_libraries(${PROJECT_NAME} PRIVATE imnodeeditor)
target_link_libraries(${PROJECT_NAME} PRIVATE imfilebrowser)
target_link_libraries(${PROJECT_NAME} PRIVATE GLEW)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE cereal)
target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
target_link_libraries(${PROJECT_NAME} PRIVATE kissfft)
target_link_libraries(${PROJECT_NAME} PRIVATE stb)
target_link_libraries(${PROJECT_NAME} PRIVATE libfluidsynth)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE GL)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

# Remove terminal window (Windows only)
if (WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Deploy resources
set(RESSOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(DEST_DIR $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources)

add_custom_target(deploy ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory ${DEST_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESSOURCE_DIR} ${DEST_DIR}
	COMMENT "Deploying resources directory to ${DEST_DIR}"
)
