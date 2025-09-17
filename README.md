# llvm-obfuscation-passes
Educational collection of LLVM 17 obfuscation passes. (Feel free to use it for your course)

NOTE: I used the skeleton and build system from https://github.com/sampsyo/llvm-pass-skeleton/

## Implemented Obfuscation Passes

### Mixed Boolean-Arithmetic (MBA) Transformations

| Pass | Description | Mathematical Identity |
|------|-------------|----------------------|
| **MBA_Add_Sub** | Obfuscates addition and subtraction using XOR/AND operations | `x + y → (x ⊕ y) + (x ∧ y) << 1` <br> `x - y → x + (-y)` |

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/your-username/llvm-obfuscation-passes.git
cd llvm-obfuscation-passes

# Build the passes
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Usage Examples
```bash
# Navigate back to project root
cd ..

# Apply MBA obfuscation to your C code
clang -fpass-plugin=build/passes/MBA_Add_Sub_Pass.so \
      examples/mba_add_sub.c -o examples/mba_add_sub_obfuscated

# View the obfuscated IR
clang -fpass-plugin=build/passes/MBA_Add_Sub_Pass.so \
      -S -emit-llvm examples/mba_add_sub.c -o examples/mba_add_sub_obfuscated.ll

# Compare original vs obfuscated
clang -S -emit-llvm examples/mba_add_sub.c -o examples/mba_add_sub_original.ll
diff examples/mba_add_sub_original.ll examples/mba_add_sub_obfuscated.ll
````
## Development

### Adding New Passes

1. Create your pass in `passes/YourPass.cpp`
2. Follow the existing MBA pass structure
3. Add to `CMakeLists.txt`
4. Create test cases in `examples/`
5. Update this README

## Contributing

Contributions are welcome! Please feel free to:

- Add new obfuscation passes
- Improve existing implementations  
- Add test cases and examples
- Fix bugs or improve documentation
- Suggest new features

## Disclaimer

This software is intended for educational and research purposes only. Users are responsible for ensuring compliance with applicable laws and regulations when using obfuscation techniques.
