// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// Contract-looking tokens after a non-function declarator must be diagnosed
// and recovered from without hitting ParseFunctionDeclaratorTail's assertion.
int value pre(true); // expected-error {{contracts may not be specified on a function pointer type}}

struct S {
  int member post(true); // expected-error {{contracts may not be specified on a function pointer type}}
};
