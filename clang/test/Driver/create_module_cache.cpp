// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
//
// RUN: not %clang -std=c++20 %t/M.cppm -fc++-modules-cache-path= -c -o %t/M.tmp.o 2>&1 | FileCheck %t/M.cppm --check-prefix=ERROR
// RUN: %clang -std=c++20 %t/M.cppm -fc++-modules-cache-path=%t/abc -c -o -
// RUN: ls %t | FileCheck %t/M.cppm --check-prefix=CHECK-AVAILABLE
// RUN: %clang -std=c++20 %t/M.cppm -fc++-modules-cache-path=%t/abc -fno-implicit-modules -c -o -
// RUN: ls %t | FileCheck %t/M.cppm --check-prefix=CHECK-AVAILABLE
//
// RUN: %clang -std=c++20 %t/Use.cpp -fc++-modules-cache-path=abc -### 2>&1 | FileCheck %t/Use.cpp
// RUN: %clang -std=c++20 %t/Use.cpp -fc++-modules-cache-path=abc -fno-implicit-modules -### 2>&1 | FileCheck %t/Use.cpp
//
// Check that the compiler will generate M-Part.pcm correctly.
// RUN: %clang -std=c++20 %t/Part.cppm -fc++-module-file-output=%t/M-Part.pcm -c -o -
// RUN: ls %t | FileCheck %t/Part.cppm --check-prefix=CHECK-AVAILABLE

//--- M.cppm
export module M;

// ERROR: unable to create default module cache path "": No such file or directory
// CHECK-AVAILABLE: abc

//--- Use.cpp
import M;

// CHECK: -fprebuilt-module-path=abc

//--- Part.cppm
export module M:Part;
// CHECK-AVAILABLE: M-Part.pcm
