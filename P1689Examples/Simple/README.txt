# (WIP) How to run

- Compiler: the in-tree clang
- Cmake: cmake master with the following patch:
> https://github.com/ChuanqiXu9/CMake/tree/ClangModule

Command line option

```
CC=clang CXX=clang++ cmake -DCMake_TEST_MODULE_COMPILATION=named,partitions,internal_partitions \
-DCMake_TEST_MODULE_COMPILATION_RULES=cxx_modules_rules_clang.cmake \
-DCMake_TEST_HOST_CMAKE=ON -S . -B build -GNinja
```

Currently, it will fail because now when the cmake compiles `importable.cxx`, it wouldn't generate the BMI `importable.pcm`.
Then when it compiles `main.cxx`, it can't find the BMI.
