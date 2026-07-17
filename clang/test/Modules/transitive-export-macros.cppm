// RUN: rm -rf %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++20 %t/A.cppm -emit-module-interface \
// RUN:   -fmodules-export-macros -o %t/A.pcm
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t %t/B.cppm \
// RUN:   -emit-module-interface -fmodules-export-macros -o %t/B.pcm
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t %t/Use.cpp \
// RUN:   -fsyntax-only -verify
//
// RUN: %clang_cc1 -std=c++20 %t/C.cppm -emit-module-interface \
// RUN:   -fmodules-export-macros -o %t/C.pcm
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t %t/D.cppm \
// RUN:   -emit-module-interface -fmodules-export-macros -o %t/D.pcm
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t %t/Use2.cpp \
// RUN:   -fsyntax-only -verify
//
// RUN: %clang_cc1 -std=c++20 %t/E.cppm -emit-module-interface -o %t/E.pcm
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t %t/Use3.cpp \
// RUN:   -fsyntax-only -verify
//
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t \
// RUN:   %t/UsePreprocessDefault.cpp -E -verify
// RUN: %clang_cc1 -std=c++20 -fprebuilt-module-path=%t \
// RUN:   %t/UsePreprocess.cpp -E -try-load-bmi-when-preprocessing \
// RUN:   | FileCheck %s --check-prefix=PP

//--- A.cppm
module;
#define VALUE 43
export module A;

//--- B.cppm
module;
import A;
export module B;

//--- Use.cpp
// expected-no-diagnostics
import B;
static_assert(VALUE == 43);

//--- C.cppm
module;
#define VALUE 43
export module C;

//--- D.cppm
module;
export module D;
export import C;

//--- Use2.cpp
// expected-no-diagnostics
import D;
static_assert(VALUE == 43);

//--- E.cppm
module;
#pragma ACC modules "export-macros"
#define PRAGMA_VALUE 44
export module E;

//--- Use3.cpp
// expected-no-diagnostics
import E;
static_assert(PRAGMA_VALUE == 44);

//--- UsePreprocessDefault.cpp
import A;
#ifndef VALUE
// expected-error@+1 {{missing module macro}}
#error missing module macro
#endif

//--- UsePreprocess.cpp
import A;
#if VALUE == 43
int preprocessing_loaded;
#else
#error missing module macro
#endif

// PP: int preprocessing_loaded;