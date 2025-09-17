# llvm-obfuscation-passes
Educational collection of LLVM obfuscation passes. (Feel free to use it for your course)

NOTE: I used the skeleton and build system from https://github.com/sampsyo/llvm-pass-skeleton/

## Supported Passes

- [x] MBA_Add_Sub.cpp : x + y -> (x ^ y) + 2*(x & y) ; x - y becomes x + (-y) in case of subtraction

## build and run
````bash
cd llvm-obfuscation-passes
mkdir build
cd build
cmake ..
make
cd ..

clang -fpass-plugin=`echo build/passes/MBA_Add_Sub_Pass.so` examples/mba_add_sub.c -o examples/mba_add_sub.o
````

