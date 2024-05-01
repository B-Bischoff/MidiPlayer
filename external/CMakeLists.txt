add_subdirectory(portmidi)
target_include_directories(portmidi PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/portmidi/pm_common
)

add_subdirectory(rtaudio)
add_subdirectory(glfw)

# GLEW
add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

include_directories(
	glew-2.2.0/include
)

add_library(GLEW STATIC
	glew-2.2.0/src/glew.c
)

# IMGUI
set(IMGUI_SOURCES
	"imgui/imgui.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/backends/imgui_impl_glfw.cpp"
	"imgui/backends/imgui_impl_opengl3.cpp"
	"imgui/backends/imgui_impl_opengl3_loader.h"

	# IMPLOT
	"implot/implot.h"
	"implot/implot_internal.h"
	"implot/implot.cpp"
	"implot/implot_items.cpp"

	# IMGUI NODE EDITOR
	"imgui-node-editor/imgui_canvas.cpp"
	"imgui-node-editor/imgui_node_editor.cpp"
	"imgui-node-editor/imgui_node_editor_api.cpp"
	"imgui-node-editor/crude_json.cpp"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
	${CMAKE_CURRENT_SOURCE_DIR}/implot
	${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor
)
