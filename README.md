Draft for implementing P1689R5 based on clang-trunk.

# Current status:

We're able to generate the P1689 format with clang-scan-deps and compile the intree examples. But we can't compile the Hello example with `ninja -j 4`, which is bad and need more looks.

# cmake version

https://github.com/ChuanqiXu9/CMake/tree/ClangModule
