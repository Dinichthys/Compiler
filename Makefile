CXXFLAGS = -I libs -D DEBUG  -ggdb -g3  -std=c++17 -Og -Wall -Wextra -pie -fPIE -Werror=vla       				\
 -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual       \
 -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal       \
 -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor \
 -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow 		   \
 -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn 	   \
 -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand   \
 -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix                      \
 -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new \
 -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer       \
 -Wlarger-than=81920 -Wstack-usage=81920

ASAN_FLAGS = -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

# // -D_FORTIFY_SOURCES=3

CXX = gcc
# CXXFLAGS = -Wall -Wextra -pedantic

MATHFLAGS = -lm

# WARNINGS = -Wall -Wextra
# # -Wswitch-enum
# ifeq ($(CXX), clang++)
#   WARNINGS += -pedantic -Wconversion -Wdangling -Wdeprecated -Wdocumentation -Wformat -Wfortify-source     \
#   -Wgcc-compat -Wgnu -Wignored-attributes -Wignored-pragmas -Wimplicit -Wmost -Wshadow-all -Wthread-safety \
#   -Wuninitialized -Wunused -Wformat
#
#   WARNINGS += -Wargument-outside-range -Wassign-enum -Wbitwise-instead-of-logical -Wc23-extensions 			\
#   -Wc11-extensions -Wcast-align -Wcast-function-type -Wcast-qual -Wcomma -Wcomment -Wcompound-token-split 	\
#   -Wconditional-uninitialized -Wduplicate-decl-specifier -Wduplicate-enum -Wduplicate-method-arg 			\
#   -Wduplicate-method-match -Wempty-body -Wempty-init-stmt -Wenum-compare -Wenum-constexpr-conversion 		\
#   -Wextra-tokens -Wfixed-enum-extension -Wfloat-equal -Wloop-analysis -Wframe-address -Wheader-guard 		\
#   -Winfinite-recursion -Wno-gnu-binary-literal -Wint-conversion -Wint-in-bool-context -Wmain 				\
#   -Wmisleading-indentation -Wmissing-braces -Wmissing-prototypes -Wover-aligned -Wundef -Wvla
# endif
# ifeq ($(CXX), cc)
#   WARNINGS += -pedantic -Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align -Wcast-qual 	\
#   -Wchar-subscripts -Wconversion -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security 			\
#   -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self 	\
#   -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn 			\
#   -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wsync-nand -Wundef -Wunreachable-code 	\
#   -Wunused -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs -Wstack-usage=8192 \
#   -Wstack-protector
# endif
# ifeq ($(CXX), g++)
#   WARNINGS += -pedantic -Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align -Wcast-qual 	\
#   -Wchar-subscripts -Wconversion -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security 			\
#   -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self 	\
#   -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn 			\
#   -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wsync-nand -Wundef -Wunreachable-code 	\
#   -Wunused -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs -Wstack-usage=8192 \
#   -Wstack-protector
# endif

CXXFLAGS += $(MATHFLAGS) $(ASAN_FLAGS)

all: front back


front: clean_dump build build_lang build_list main_frontend.o struct_lang.o parse_flags_lang.o parse_mode_lang.o read_lang.o dump_lang.o connect_tree_lang.o write_tree_lang.o list_construction.o list_index.o list_push.o list_pop.o my_stdio.o logging.o print_error.o
	@$(CXX) $(CXXFLAGS) build/main_frontend.o build/lang/*.o build/list/*.o build/my_stdio.o build/logging.o build/print_error.o -o front

back: clean_dump build build_lang build_list main_backend.o struct_lang.o parse_flags_lang.o parse_mode_lang.o read_lang.o backend_lang.o dump_lang.o connect_tree_lang.o read_tree_lang.o my_stdio.o logging.o print_error.o
	@$(CXX) $(CXXFLAGS) build/main_backend.o build/lang/*.o build/list/*.o build/my_stdio.o build/logging.o build/print_error.o -o back


build:
	mkdir -p build

build_lang:
	mkdir -p build/lang

build_list:
	mkdir -p build/list


my_stdio.o: libs/My_lib/My_stdio/my_stdio.cpp
	@$(CXX) $(CXXFLAGS) -c libs/My_lib/My_stdio/my_stdio.cpp -o build/my_stdio.o

logging.o: libs/My_lib/Logger/logging.cpp
	@$(CXX) $(CXXFLAGS) -c libs/My_lib/Logger/logging.cpp -o build/logging.o

print_error.o: libs/My_lib/Assert/print_error.cpp
	@$(CXX) $(CXXFLAGS) -c libs/My_lib/Assert/print_error.cpp -o build/print_error.o


main_frontend.o: src/main_frontend.cpp
	@$(CXX) $(CXXFLAGS) -c src/main_frontend.cpp -o build/main_frontend.o

main_backend.o: src/main_backend.cpp
	@$(CXX) $(CXXFLAGS) -c src/main_backend.cpp -o build/main_backend.o


main_lang.o: src/main_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/main_lang.cpp -o build/lang/main_lang.o

struct_lang.o: src/struct_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/struct_lang.cpp -o build/lang/struct_lang.o

parse_flags_lang.o: src/parse_flags_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/parse_flags_lang.cpp -o build/lang/parse_flags_lang.o

parse_mode_lang.o: src/parse_mode_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/parse_mode_lang.cpp -o build/lang/parse_mode_lang.o

read_lang.o: src/read_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/read_lang.cpp -o build/lang/read_lang.o

backend_lang.o: src/backend_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/backend_lang.cpp -o build/lang/backend_lang.o

dump_lang.o: src/dump_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/dump_lang.cpp -o build/lang/dump_lang.o

connect_tree_lang.o: src/connect_tree_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/connect_tree_lang.cpp -o build/lang/connect_tree_lang.o

write_tree_lang.o: src/write_tree_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/write_tree_lang.cpp -o build/lang/write_tree_lang.o

read_tree_lang.o: src/read_tree_lang.cpp
	@$(CXX) $(CXXFLAGS) -c src/read_tree_lang.cpp -o build/lang/read_tree_lang.o


list_construction.o: src/list/list_construction.cpp
	@g++ $(CXXFLAGS) -c src/list/list_construction.cpp -o build/list/list_construction.o

list_push.o: src/list/list_push.cpp
	@g++ $(CXXFLAGS) -c src/list/list_push.cpp -o build/list/list_push.o

list_pop.o: src/list/list_pop.cpp
	@g++ $(CXXFLAGS) -c src/list/list_pop.cpp -o build/list/list_pop.o

list_index.o: src/list/list_index.cpp
	@g++ $(CXXFLAGS) -c src/list/list_index.cpp -o build/list/list_index.o


clean: rmdir_build
	rm -rf lang

clean_dump:
	rm -rf log/*

rmdir_build: clean_build
	rmdir build

clean_build: rmdir_lang rmdir_list
	rm -rf build/*

rmdir_lang: clean_lang
	rmdir build/lang

clean_lang:
	rm -rf build/lang/*

rmdir_list: clean_list
	rmdir build/list

clean_list:
	rm -rf build/list/*


frontend:
	@./front -i Program.to -l log/log.txt -o DataBase.piym

backend:
	@./back -i DataBase.piym -l log/log.txt -o src/SPU/Files_.asm/program.asm


disasm:
	@src/SPU/compiler src/SPU/program_code.txt src/SPU/Files_.asm/program.asm

compile:
	@src/SPU/compiler src/SPU/Files_.asm/program.asm src/SPU/program_code.txt


run:
	@src/SPU/processor src/SPU/program_code.txt
