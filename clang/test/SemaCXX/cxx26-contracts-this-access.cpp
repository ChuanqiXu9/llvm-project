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

// P2900R14 §3.4.1: Contract predicates can access 'this' and members
// Delayed parsing is now implemented, so these should all work.

struct S {
  int x;
  int y;

  // Member function preconditions - should all work now
  void f1() pre(this->x > 0);
  void f2() pre(x > 0);
  void f3() pre(this->x > 0 && this->y > 0);

  // Member function postconditions - should all work now
  int g1() post(this->x > 0);
  int g2() post(r: r > this->x);

  // Const member functions - should work now
  void h1() const pre(this->x > 0);
  void h2() const pre(x > 0);

  // Static member functions - no 'this' available, so normal error
  static void s1() pre(x > 0);  // expected-error {{invalid use of non-static data member 'x'}}
};

struct U {
  int x;
  ~U() pre(x >= 0);
};

struct RedeclOK {
  void f(int x) pre(x > 0);
};

void RedeclOK::f(int x) pre(x > 0) {}

struct RedeclMismatch {
  void f(int x) pre(x > 0); // expected-note {{previous declaration is here}}
};

void RedeclMismatch::f(int x) pre(x > 1) {} // expected-error {{contracts on function redeclaration do not match the previous declaration}}

struct RedeclLambdaOK {
  void f() pre([] { return true; }());
};

void RedeclLambdaOK::f() pre([] { return true; }()) {}

struct EmptyPredicate {
  void f() pre(); // expected-error {{expected expression}}
  void g() pre(this->x > 0);
  int x;
};

struct InvalidDelayedContract {
  void f(int x) pre(++x > 0) pre(x > 0); // expected-error {{contract predicate cannot modify variable}} expected-note {{previous declaration is here}}
};

void InvalidDelayedContract::f(int x) pre(x > 1) {} // expected-error {{contracts on function redeclaration do not match the previous declaration}}
