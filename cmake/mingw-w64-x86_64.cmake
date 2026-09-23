# Optional convenience toolchain file for cross-compiling the plugin DLL
# from Linux/macOS using mingw-w64, if you'd rather not set up Visual
# Studio just to build a DLL. The DLL still only runs inside Notepad++ on
# Windows - this just lets you produce it without a Windows machine.
#
# Usage:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake
#   cmake --build build
#
# Requires: sudo apt install mingw-w64 g++-mingw-w64-x86-64   (Debian/Ubuntu)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
