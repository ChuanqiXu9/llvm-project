// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -ast-print %s | FileCheck %s

int source_order(const int x) post(result: result >= x) pre(x > 0)
    post(result: result < 100) pre(x != 42);

// CHECK: int source_order(const int x) post(result: result >= x) pre(x > 0) post(result: result < 100) pre(x != 42);
