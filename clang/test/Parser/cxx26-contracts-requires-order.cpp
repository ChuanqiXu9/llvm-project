// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -fsyntax-only -verify %s

template <typename>
concept Constrained = true;

template <typename T>
int declaration(T value) pre(value > T{}) requires Constrained<T>;
// expected-error@-1 {{trailing requires clause must appear before contract specifiers}}

template <typename T>
int definition(T value) pre(value > T{}) requires Constrained<T> {
// expected-error@-1 {{trailing requires clause must appear before contract specifiers}}
  return value;
}

struct Member {
  template <typename T>
  int f(T value) pre(value > T{}) requires Constrained<T>;
  // expected-error@-1 {{trailing requires clause must appear before contract specifiers}}
};

void lambdas() {
  auto lambda = []<typename T>(T value)
    pre(value > T{}) requires Constrained<T> {
// expected-error@-1 {{trailing requires clause must appear before contract specifiers}}
    return value;
  };
  (void)lambda;
}
