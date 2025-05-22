cmake --build build
mkdir -p DataBase
mkdir -p DataBase/objects
mkdir -p log
nasm -f elf64 -l build/lib.lst to_libs/lib.asm -o DataBase/objects/lib.o -g -F dwarf
nasm -f elf64 -l build/printf.lst to_libs/printf.asm -o DataBase/objects/printf.o -g -F dwarf
sh scripts/frontend.sh
sh scripts/middleend.sh
sh scripts/backendNASM.sh
nasm -f elf64 -l build/nasm.lst DataBase/program_nasm.asm -o DataBase/objects/nasm.o -g -F dwarf
ld DataBase/objects/printf.o DataBase/objects/lib.o DataBase/objects/nasm.o -o DataBase/program_nasm
./DataBase/program_nasm
