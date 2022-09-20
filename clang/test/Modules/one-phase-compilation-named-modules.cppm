// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
//
// RUN: %clang -std=c++20 %t/M.cppm -c -o - -fmodules-cache-path=%t/pcm.cache
// RUN: %clang -std=c++20 %t/Use.cpp -fsyntax-only  -fprebuilt-module-path=%t/pcm.cache -Xclang -verify
//
// RUN: rm -f %t/M.pcm
// RUN: %clang -std=c++20 %t/MismatchedName.cppm -c -o - -fmodules-cache-path=%t/pcm.cache -fmodule-name=M
// RUN: %clang -std=c++20 %t/Use.cpp -fsyntax-only  -fprebuilt-module-path=%t/pcm.cache -Xclang -verify

//--- M.cppm
export module M;
export int getValue();

//--- MismatchedName.cppm
export module M;
export int getValue();

//--- Use.cpp
// expected-no-diagnostics
import M;
int Use() {
    return getValue();
}
