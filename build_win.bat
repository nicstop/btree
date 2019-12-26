@ECHO OFF
clang main.c -o build/btree_test.exe -std=C89 -O0 -g -gcodeview -ansi -pedantic
