// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify=enforce %s
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -fsyntax-only -verify=ignore %s

// ignore-no-diagnostics

// Test: Missing std::contracts namespace entirely

int f1(int x) pre(x > 0); // enforce-error {{cannot use contract assertions: namespace 'std::contracts' not found; include <contracts> to use contract assertions}}

void f2(int x) {
  contract_assert(x > 0);
}
