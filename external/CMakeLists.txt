# GLFW
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build GLFW with Wayland support" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)

# GLEW
add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

add_library(GLEW STATIC
	glew-2.2.0/src/glew.c
)

target_include_directories(GLEW PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glew-2.2.0/include)

# IMGUI
set(IMGUI_SOURCES
	"imgui/imgui.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/backends/imgui_impl_glfw.cpp"
	"imgui/backends/imgui_impl_opengl3.cpp"
	"imgui/backends/imgui_impl_glfw.h"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
	${CMAKE_CURRENT_SOURCE_DIR}/glfw
	${CMAKE_CURRENT_SOURCE_DIR}/imgui-notify/unixExample/backends
	${CMAKE_CURRENT_SOURCE_DIR}/imgui-notify/unixExample/fonts
)
target_link_libraries(imgui PUBLIC glfw)

# IMPLOT
set(IMPLOT_SOURCES
	"implot/implot.cpp"
	"implot/implot_items.cpp"
)
add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/implot
)
target_link_libraries(implot PUBLIC imgui)

# IMGUI NODE EDITOR
set(IMNODE_EDITOR_SOURCES
	"imgui-node-editor/imgui_canvas.cpp"
	"imgui-node-editor/imgui_node_editor.cpp"
	"imgui-node-editor/imgui_node_editor_api.cpp"
	"imgui-node-editor/crude_json.cpp"
)
add_library(imnodeeditor STATIC ${IMNODE_EDITOR_SOURCES})
target_include_directories(imnodeeditor PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor
)
target_link_libraries(imnodeeditor PUBLIC imgui)

# IMGUI FILEBROWSER
add_library(imfilebrowser INTERFACE)
target_include_directories(imfilebrowser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/imgui-filebrowser)

#CEREAL
add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cereal/include)

# RTAUDIO
# Conflicts with glfw uninstall target
set(RTAUDIO_TARGETNAME_UNINSTALL "rtaudio_uninstall" CACHE STRING "Name of 'uninstall' build target" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/rtaudio)

#PORTMIDI
add_subdirectory(portmidi)
target_include_directories(portmidi PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/portmidi/pm_common>
	$<INSTALL_INTERFACE:include/portmidi/pm_common> # installation is not planned but this will avoid futur error
)

# KISSFFT
set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)
set(KISSFFT_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(kissfft)

# STB
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# FLUIDSYNTH
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(ENABLE_SHARED OFF CACHE BOOL "Disable shared lib" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "Enable static lib" FORCE)
set(ENABLE_FLUIDSYNTH_TOOLS OFF CACHE BOOL "Disable fluidsynth CLI build" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fluidsynth)

# TINYSOUNDFONT
add_library(tinysoundfont INTERFACE)
target_include_directories(tinysoundfont INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinysoundfont)
