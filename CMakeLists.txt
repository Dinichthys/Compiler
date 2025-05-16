cmake_minimum_required (VERSION 3.20)
project (language_to CXX ASM_NASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # to generate compile_commands.json

add_subdirectory(frontend)
add_subdirectory(backend)
add_subdirectory(SPU)
add_subdirectory(libs)
add_subdirectory(flags)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
