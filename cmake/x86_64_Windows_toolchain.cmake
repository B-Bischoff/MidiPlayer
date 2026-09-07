set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -static-libgcc -pthread")
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_AR x86_64-w64-mingw32-gcc-ar-posix)
set(CMAKE_LINKER x86_64-w64-mingw32-ld)
set(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)

# GCC >= 14 (e.g. Debian trixie's mingw-w64 toolchain) turns old-style
# implicit-int / incompatible-pointer-type constructs into hard errors for C.
# The vendored portmidi Windows backend (external/pm_win) relies on this
# pre-C23 behavior, so downgrade these back to warnings for C sources only.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=implicit-int -Wno-error=incompatible-pointer-types -Wno-error=int-conversion")
