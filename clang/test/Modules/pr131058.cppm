// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++20 %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

// RUN: %clang_cc1 -std=c++20 %t/M.cppm -emit-reduced-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

// RUN: %clang_cc1 -std=c++20 %t/M0.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t
// RUN: %clang_cc1 -std=c++20 %t/M0.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

// RUN: %clang_cc1 -std=c++20 %t/M0.cppm -emit-reduced-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t
// RUN: %clang_cc1 -std=c++20 %t/M0.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

// RUN: %clang_cc1 -std=c++20 %t/M2.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

// RUN: %clang_cc1 -std=c++20 %t/M2.cppm -emit-reduced-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++20 %t/use.cpp -fsyntax-only -verify -fprebuilt-module-path=%t

//--- enum.h
enum {    SomeName,    };

//--- M.cppm
module;
#include "enum.h"
export module M;
export auto e = SomeName;

//--- M0.cppm
export module M;
enum {    SomeName,    };
export auto e = SomeName;

//--- M0.cpp
// expected-no-diagnostics
module M;
auto a = SomeName;

//--- use.cpp
import M;
auto a = SomeName; // expected-error {{use of undeclared identifier 'SomeName'}}
auto b = decltype(e)::SomeName; 

//--- enum1.h
extern "C++" {
enum {    SomeName,    };
}

//--- M2.cppm
module;
#include "enum1.h"
export module M;
export auto e = SomeName;

