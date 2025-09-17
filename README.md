# llvm-obfuscation-passes
Educational collection of LLVM obfuscation passes. (Feel free to use it for your course)

NOTE: I used the skeleton and build system from https://github.com/sampsyo/llvm-pass-skeleton/

## Supported Passes

- [x] MBA Add : x + y -> (x ^ y) + 2*(x & y)

## build and run
````bash
cd llvm-pass-skeleton
mkdir build
cd build
cmake ..
make
cd ..

clang -fpass-plugin=`echo build/passes/MBA_Add_Pass.so` examples/mba_add.c -o examples/mba_add.o
````
