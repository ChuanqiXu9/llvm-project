// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
// RUN: cd %t
//
// RUN: not %clang -std=c++20 M.cppm -fmodules-cache-path=abc/def/ghi -c -o -
// RUN: %clang -std=c++20 M.cppm -fmodules-cache-path=abc -c -o -
// RUN: ls %t | FileCheck M.cppm --check-prefix=CHECK-AVAILABLE
//
// RUN: %clang -std=c++20 %t/Use.cpp -fmodules-cache-path=abc -### 2>&1 | FileCheck Use.cpp

//--- M.cppm
export module M;

// ERROR: unable to create default module cache path "abc/def/ghi": No such file or directory
// CHECK-AVAILABLE: abc

//--- Use.cpp
import M;

// CHECK: -fprebuilt-module-path=abc
