// RUN: not %clang_cc1 -std=c17 -fcontracts -fsyntax-only %s 2>&1 | FileCheck %s

int f(int x) pre(x);

// CHECK: error: expected function body after function declarator
