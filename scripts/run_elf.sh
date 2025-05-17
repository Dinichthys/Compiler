cmake --build build
mkdir -p DataBase
mkdir -p DataBase/objects
mkdir -p log
nasm -f elf64 -l build/lib.lst to_libs/lib.asm -o DataBase/objects/lib.o -g -F dwarf
nasm -f elf64 -l build/printf.lst to_libs/printf.asm -o DataBase/objects/printf.o -g -F dwarf
sh scripts/frontend.sh
sh scripts/backendELF.sh
./program_elf
