// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  class contract_violation {
    const char* _M_file;
    const char* _M_function;
    const char* _M_comment;
    unsigned int _M_line;
    contract_kind _M_kind;
    detection_mode_t _M_detection_mode;
  };
  void handle_contract_violation(const contract_violation&);
}

// === 1. Predicate bool conversion ===

struct NoBool {};
struct HasBool { explicit operator bool() const; };

int f1(int x) pre(x > 0); // ok: comparison returns bool

int f2(int x) pre(x);     // ok: int contextually converts to bool

void f3(int x) {
  contract_assert(x);      // ok
}

int f4(NoBool nb) pre(nb); // expected-error {{value of type 'NoBool' is not contextually convertible to 'bool'}}

int f5(HasBool hb) pre(hb); // ok: explicit operator bool allowed

void f6(NoBool nb) {
  contract_assert(nb); // expected-error {{value of type 'NoBool' is not contextually convertible to 'bool'}}
}

// === 2. Redeclaration consistency ===

int good_redecl(int x) pre(x > 0);
int good_redecl(int x); // ok: omits contracts (inherits)

int mismatch(int x) pre(x > 0);       // expected-note {{previous declaration is here}}
int mismatch(int x) pre(x > 1);       // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int no_contract_first(int x);              // expected-note {{previous declaration is here}}
int no_contract_first(int x) pre(x > 0);  // expected-error {{contracts may not be added on a function redeclaration}}

int same_contracts(const int x) pre(x > 0) post(r: r >= 0);
int same_contracts(const int x) pre(x > 0) post(r: r >= 0); // ok: identical

int different_count(int x) pre(x > 0);     // expected-note {{previous declaration is here}}
int different_count(int x) pre(x > 0) pre(x < 100); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int post_mismatch(const int x) post(r: r > 0); // expected-note {{previous declaration is here}}
int post_mismatch(const int x) post(r: r > 1); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

// === 3. Virtual functions not supported ===

struct Base {
  virtual int compute(int x) pre(x > 0); // expected-error {{contracts may not be specified on a virtual function at this time}} \
                                         // expected-note {{overridden virtual function is here}}
  virtual void action(); // ok: no contracts
};

struct Derived : Base {
  // Override functions with contracts are properly rejected
  int compute(int x) pre(x > 0) override; // expected-error {{contracts may not be specified on an overriding function}}
  void action() override; // ok: no contracts on override
};

// === 4. Template instantiation ===

template <typename T>
T clamp(T x, const T lo, const T hi) pre(lo <= hi) post(r: r >= lo) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

void use_clamp() {
  clamp(5, 0, 10);
  clamp(3.0, 1.0, 9.0);
}

template <typename T>
T must_positive(T x) pre(x > T{}) {
  return x;
}

void use_must_positive() {
  must_positive(42);
  must_positive(3.14);
}
