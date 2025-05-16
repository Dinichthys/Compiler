cmake --build build
mkdir -p DataBase
mkdir -p DataBase/objects
mkdir -p log
sh scripts/frontend.sh
sh scripts/backendSPU.sh
sh scripts/assembler.sh
sh scripts/spu.sh
