add_executable (frontend
    source/main_frontend.cpp

    source/read_lang.cpp
    source/write_tree_lang.cpp
)

target_link_libraries(frontend
    all_libs
)

target_include_directories (frontend
    PRIVATE
        ./include
        ../
        ../libs
)

target_compile_features (frontend
    PRIVATE
        cxx_std_17
)

target_compile_options (frontend
    PRIVATE
        -fdiagnostics-color=always

        -Wall
        -Wextra
        -fstack-protector-strong
        -fcheck-new
        -fstrict-overflow

        -march=native

        $<$<CONFIG:Debug>:
            -Og
            -g
            -g3
            -DDEBUG
            -ggdb
            -fsanitize=address,leak,undefined
        >

        $<$<CONFIG:Release>:
            -O2
            -flto
            -DNDEBUG
        >
)

target_link_options (frontend
    PRIVATE
        -march=native

        $<$<CONFIG:Debug>:
            -Og
            -g
            -g3
            -DDEBUG
            -ggdb
            -fsanitize=address,leak,undefined
        >

        $<$<CONFIG:Release>:
            -O2
            -flto
            -DNDEBUG
        >
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # to generate compile_commands.json

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
