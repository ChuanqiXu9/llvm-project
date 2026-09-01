// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

namespace std {
  struct source_location {
    struct __impl {
      const char* _M_file_name;
      const char* _M_function_name;
      unsigned _M_line;
      unsigned _M_column;
    };
  };
}

namespace std::contracts {
  enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
  enum class evaluation_semantic : unsigned short {
    ignore = 1,
    observe = 2,
    enforce = 3,
    quick_enforce = 4
  };
  enum class detection_mode : unsigned short {
    predicate_false = 1,
    evaluation_exception = 2
  };
  class contract_violation {
    unsigned short _M_version;
    assertion_kind _M_assertion_kind;
    evaluation_semantic _M_evaluation_semantic;
    detection_mode _M_detection_mode;
    const char* _M_comment;
    const void* _M_src_loc_ptr;
    void* _M_ext;
  };
}
void handle_contract_violation(const std::contracts::contract_violation&);

// P2900R14 §3.4.4: Parameters used in postcondition predicates must be const-qualified

int f(int i) post(i != 0);  // expected-error {{parameter 'i' used in a postcondition predicate must be const-qualified}}
int g(const int i) post(i != 0);  // OK
int h(int i) post(r: r > i);  // expected-error {{parameter 'i' used in a postcondition predicate must be const-qualified}}
int k(const int i) post(r: r > i);  // OK
int m(int a, const int b) post(a > b);  // expected-error {{parameter 'a' used in a postcondition predicate must be const-qualified}}
int n(const int a, const int b) post(a > b);  // OK

// Reference parameters are exempt: only a non-reference parameter odr-used by
// a postcondition must be const-qualified.
struct RefBox { int n; };
int ref_ok(const int &i) post(r: r > i);  // OK
int ref_bad(int &i) post(r: r > i);  // OK
int ref_member_ok(const RefBox &box) post(r: r >= box.n);  // OK
int ref_member_bad(RefBox &box) post(r: r >= box.n);  // OK
int rvalue_ref_ok(const int &&i) post(r: r > i);  // OK
int rvalue_ref_bad(int &&i) post(r: r > i);  // OK

// Parameters not used in postcondition don't need to be const
int p(int i) post(r: r > 0);  // OK - i not used in predicate
int q(int i, int j) post(r: r > 0);  // OK - neither used

// Multiple postconditions
int s(int a, const int b) post(a > 0) post(b > 0);  // expected-error {{parameter 'a' used in a postcondition predicate must be const-qualified}}
int t(const int a, const int b) post(a > 0) post(b > 0);  // OK

// Precondition doesn't require const
int u(int i) pre(i > 0) post(r: r > 0);  // OK - i not used in postcondition

int redeclared_const(const int value) post(value > 0);
int redeclared_const(int renamed); // expected-error {{parameter 'renamed' used in a postcondition predicate must be const-qualified}}

// An omitted contract declaration inherits a predicate that still refers to
// the parameter declaration which originally carried the contract.
int redeclared_const_chain(const int value) post(value > 0);
int redeclared_const_chain(const int middle);
int redeclared_const_chain(int final); // expected-error {{parameter 'final' used in a postcondition predicate must be const-qualified}}

// A lambda parameter at the same index is not a parameter of the contracted
// function and must not trigger the const requirement.
int redeclared_lambda_param(int value)
    post([](int value) { return value > 0; }(1));
int redeclared_lambda_param(int value);
int redeclared_lambda_param(int value);

// An unevaluated reference is not an odr-use.
int unevaluated(int value) post(sizeof(value) == sizeof(int));
