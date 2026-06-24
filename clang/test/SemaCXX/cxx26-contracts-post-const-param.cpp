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

// P2900R14 §3.4.4: Parameters used in postcondition predicates must be const-qualified

int f(int i) post(i != 0);  // expected-error {{parameter 'i' used in a postcondition predicate must be const-qualified}}
int g(const int i) post(i != 0);  // OK
int h(int i) post(r: r > i);  // expected-error {{parameter 'i' used in a postcondition predicate must be const-qualified}}
int k(const int i) post(r: r > i);  // OK
int m(int a, const int b) post(a > b);  // expected-error {{parameter 'a' used in a postcondition predicate must be const-qualified}}
int n(const int a, const int b) post(a > b);  // OK

// Parameters not used in postcondition don't need to be const
int p(int i) post(r: r > 0);  // OK - i not used in predicate
int q(int i, int j) post(r: r > 0);  // OK - neither used

// Multiple postconditions
int s(int a, const int b) post(a > 0) post(b > 0);  // expected-error {{parameter 'a' used in a postcondition predicate must be const-qualified}}
int t(const int a, const int b) post(a > 0) post(b > 0);  // OK

// Precondition doesn't require const
int u(int i) pre(i > 0) post(r: r > 0);  // OK - i not used in postcondition
