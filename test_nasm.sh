cmake --build build
nasm -f elf64 -l build/lib.lst lib.asm -o DataBase/objects/lib.o -g -F dwarf
sh frontend.sh
sh backendNASM.sh
nasm -f elf64 -l build/nasm.lst program_nasm.asm -o DataBase/objects/nasm.o -g -F dwarf
ld DataBase/objects/printf.o DataBase/objects/lib.o DataBase/objects/nasm.o -o program
