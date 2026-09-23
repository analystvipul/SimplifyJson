cmake_minimum_required(VERSION 3.15)
project(NppJsonTools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR
        "This project builds a Notepad++ plugin DLL and must be built "
        "either on Windows with MSVC, or cross-compiled from Linux with "
        "mingw-w64, e.g.:\n"
        "  cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake\n"
        "  cmake --build build")
endif()

add_library(NppJsonTools SHARED
    src/PluginDefinition.cpp
    src/NppJsonTools.cpp
    src/JsonUtils.cpp
)

target_compile_definitions(NppJsonTools PRIVATE UNICODE _UNICODE)

# Notepad++/Scintilla headers live alongside the source in src/.
target_include_directories(NppJsonTools PRIVATE src)

if (MSVC)
    target_compile_options(NppJsonTools PRIVATE /W4 /EHsc)
else()
    target_compile_options(NppJsonTools PRIVATE -Wall -Wextra)
    target_link_options(NppJsonTools PRIVATE -static-libgcc -static-libstdc++)
endif()

set_target_properties(NppJsonTools PROPERTIES
    OUTPUT_NAME "SimplifyJson"
    PREFIX ""
    SUFFIX ".dll"
)
