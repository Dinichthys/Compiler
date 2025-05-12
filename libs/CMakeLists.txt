add_subdirectory (PYAM_IR)

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
    tree/source/write_tree_lang.cpp
    tree/source/struct_lang.cpp
)

target_link_libraries(all_libs
    pyam_ir
    flags
)

target_include_directories(all_libs
    PUBLIC
        .
        ../

        ./flags/include

        ./List/include

        ./MyLib
        ./MyLib/My_stdio
        ./MyLib/Logger
        ./MyLib/Assert

        ./tree/include
)

# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
# cmake --build build
