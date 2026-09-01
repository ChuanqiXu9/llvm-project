// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore \
// RUN:   %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore \
// RUN:   %t/M-impl.cpp -fmodule-file=M=%t/M.pcm -fsyntax-only -verify
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore \
// RUN:   -x c++-header %t/PCH.h -emit-pch -o %t/PCH.pch
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore \
// RUN:   -include-pch %t/PCH.pch %t/UsePCH.cpp -fsyntax-only -verify

//--- M.cppm
export module M;

export int from_module(const int value) post(value > 0);

//--- M-impl.cpp
module M;

int from_module(const int middle);
int from_module(int final); // expected-error {{parameter 'final' used in a postcondition predicate must be const-qualified}}

//--- PCH.h
int from_pch(const int value) post(value > 0);

//--- UsePCH.cpp
int from_pch(const int middle);
int from_pch(int final); // expected-error {{parameter 'final' used in a postcondition predicate must be const-qualified}}
