add_executable (backend_spu
    source/main_backend.cpp
    source/translationSPU.cpp
)

target_link_libraries (backend_spu
    all_libs
    pyam_ir
    flags
)

target_include_directories (backend_spu
    PRIVATE
        include/
        ../../SPU/libs
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
