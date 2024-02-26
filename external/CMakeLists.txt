add_subdirectory(portmidi)
target_include_directories(portmidi PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/portmidi/pm_common
)

add_subdirectory(rtaudio)

# IMGUI
set(IMGUI_SOURCES
	"imgui/imgui.cpp"

	"imgui/imgui_draw.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/backends/imgui_impl_glfw.cpp"
	"imgui/backends/imgui_impl_opengl3.cpp"
	"imgui/backends/imgui_impl_opengl3_loader.h"
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)
