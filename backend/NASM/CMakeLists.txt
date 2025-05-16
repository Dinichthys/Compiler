add_executable (backend_nasm
    source/main_backend.cpp
    source/translationNASM.cpp
)

target_link_libraries(backend_nasm
    all_libs
    pyam_ir
    flags
)

target_include_directories (backend_nasm
    PRIVATE
        include/
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
