# (WIP) How to run

- Compiler: the in-tree clang
- Cmake: cmake master with the following patch:
> https://gitlab.kitware.com/ben.boeckel/cmake/-/commit/3d1e6afed1c5e3b33f2f5b31a3a87c6d02e158e4

Command line option

```
CC=clang CXX=clang++ cmake -DCMake_TEST_MODULE_COMPILATION=named \
-DCMake_TEST_MODULE_COMPILATION_RULES=cxx_modules_rules_clang.cmake \
-DCMake_TEST_HOST_CMAKE=ON -S . -B build -GNinja
```

Currently, it will meet the following error:

```
$ninja
[1/3] Building CXX object CMakeFiles/simple.dir/importable.cxx.o
FAILED: CMakeFiles/simple.dir/importable.cxx.o CMakeFiles/simple.dir/importable.pcm
clang++   -std=c++20 -MD -MT CMakeFiles/simple.dir/importable.cxx.o -MF CMakeFiles/simple.dir/importable.cxx.o.d @CMakeFiles/simple.dir/importable.cxx.o.modmap -fdep-format=trtbd -x c++ -std=c++20 -o CMakeFiles/simple.dir/importable.cxx.o -c Simple/importable.cxx
error: -fdep-format= requires -fdep-file=
error: -fdep-format= requires -fdep-output=
ninja: build stopped: subcommand failed.
```


