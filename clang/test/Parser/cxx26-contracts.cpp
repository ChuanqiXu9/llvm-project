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

int divide(int a, int b) pre(b != 0);

int square(int x) post(r: r >= 0);

int abs_val(int x) pre(x >= -1000) pre(x <= 1000) post(r: r >= 0);

int safe_div(const int a, const int b) pre(b != 0) post(r: r * b == a);

// Multiple post with result names
int clamp(int x) post(r: r >= 0) post(r: r <= 100);

// post without result name
int identity(const int x) post(x >= 0);

void f(int x) {
  contract_assert(x > 0);
}

// pre and post are not keywords - they can still be used as identifiers.
int pre = 42;
int post(int x) { return x; }
void use() {
  pre = 10;
  post(5);
}

// post(name: expr) on void-returning function is an error.
void g() post(r: r > 0); // expected-error {{post-condition result name on function returning void}} \
                          // expected-error {{use of undeclared identifier 'r'}}

