add_executable (backend_elf
    source/main_backend.cpp
    source/translationELF.cpp
)

target_link_libraries (backend_elf
    all_libs
    pyam_ir
    flags
)

target_include_directories (backend_elf
    PRIVATE
        include/
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
