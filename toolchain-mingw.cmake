set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10)

# Specify the cross-compiling toolchain
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Specify the target architecture
set(CMAKE_C_FLAGS "-march=x86-64")
set(CMAKE_CXX_FLAGS "-march=x86-64")

# Set installation directories if needed
# set(CMAKE_INSTALL_PREFIX "/path/to/installation/directory")
