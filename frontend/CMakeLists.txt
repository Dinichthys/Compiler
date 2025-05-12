add_executable (frontend
    source/main_frontend.cpp

    source/read_lang.cpp
    source/write_ir.cpp
    source/lexer.cpp
    source/parser.cpp

    include/Stack/stack.cpp
    include/Stack/hash.cpp
)

target_link_libraries(frontend
    all_libs
    pyam_ir
    flags
)

target_include_directories (frontend
    PRIVATE
        ./include
        ../
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
