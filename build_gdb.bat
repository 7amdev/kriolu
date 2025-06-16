@echo off

if exist "build_gdb" rmdir /S /Q build_gdb
mkdir build_gdb
pushd build_gdb
gcc -ggdb -o kriolu.exe ../src/*.c 
popd
