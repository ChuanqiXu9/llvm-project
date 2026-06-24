// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  class contract_violation {
    const char* _M_file; const char* _M_function; const char* _M_comment;
    unsigned int _M_line; contract_kind _M_kind; detection_mode_t _M_detection_mode;
  };
  void handle_contract_violation(const contract_violation&);
}

struct Foo {
  Foo() pre(true) {} // expected-error {{contracts may not be specified on a constructor}}
  Foo(int x) pre(x > 0) {} // expected-error {{contracts may not be specified on a constructor}}
  Foo(double x) pre(x > 0) {} // expected-error {{contracts may not be specified on a constructor}}
  ~Foo() pre(true) {} // expected-error {{contracts may not be specified on a destructor}}
};

struct Bar {
  Bar() = default;
  ~Bar() = default;
};

struct Baz {
  Baz(int x) pre(x > 0); // expected-error {{contracts may not be specified on a constructor}}
  ~Baz() post(true); // expected-error {{contracts may not be specified on a destructor}}
};

Baz::Baz(int x) {} // OK - definition without contracts
Baz::~Baz() {} // OK - definition without contracts

struct Valid {
  Valid(int x) {} // OK
  ~Valid() {} // OK
  int f(int x) pre(x > 0) { return x; } // OK
};
