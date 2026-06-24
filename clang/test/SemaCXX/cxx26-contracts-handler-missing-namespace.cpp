// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// Test: Missing std::contracts namespace entirely

int f1(int x) pre(x > 0); // expected-error {{cannot use contract assertions: namespace 'std::contracts' not found; include <contracts> to use contract assertions}}
