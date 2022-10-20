# How to generate the P1689 format

```
clang-scan-deps --format=p1689 --compilation-database=compile_commands.json
```

# How to run (Unable to support)

- Compiler: the in-tree clang
- Cmake: cmake master with the following patch:
> https://github.com/ChuanqiXu9/CMake/tree/ClangModule

Command line option

```
CC=clang CXX=clang++ cmake -DCMake_TEST_MODULE_COMPILATION=named,partitions,internal_partitions \
-DCMake_TEST_MODULE_COMPILATION_RULES=cxx_modules_rules_clang.cmake \
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMake_TEST_HOST_CMAKE=ON -S . -B build -GNinja

cd build
ninja -v
./simple
```
