add_executable (backend
    source/main_backend.cpp

    source/read_tree_lang.cpp
    source/backend_lang.cpp
    source/translationSPU.cpp
    source/translationNASM.cpp
)

target_link_libraries(backend
    all_libs
    flags
)

target_include_directories (backend
    PRIVATE
        ./include
        ../SPU/libs
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
