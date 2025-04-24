add_library (all_libs
    STATIC

    flags/source/parse_flags_lang.cpp
    flags/source/parse_mode_lang.cpp

    List/source/list_construction.cpp
    List/source/list_error_handler.cpp
    List/source/list_src.cpp
    List/source/list_pop.cpp
    List/source/list_push.cpp

    MyLib/Logger/logging.cpp
    MyLib/Assert/print_error.cpp
    MyLib/My_stdio/my_stdio.cpp

    tree/source/connect_tree_lang.cpp
    tree/source/dump_lang.cpp
    tree/source/struct_lang.cpp
)

target_include_directories(all_libs
    PUBLIC
        .
        ../

        ../frontend/include
        ../backend/include

        ./flags/include

        ./List/include

        ./MyLib
        ./MyLib/My_stdio
        ./MyLib/Logger
        ./MyLib/Assert

        ./tree/include
)

target_compile_features (all_libs
    PRIVATE
        cxx_std_17
)

target_compile_options (all_libs
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

target_link_options (all_libs
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
