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

# STB
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# TINYSOUNDFONT
add_library(tinysoundfont INTERFACE)
target_include_directories(tinysoundfont INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinysoundfont)
