// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s
// Tests based on P2900R14 examples not covered by other test files.

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

// === §3.3.6: Function pointer with specifiers (ill-formed) ===

int func_ptr_target(int x) post(r: r != 0);
int (*fp1)(int) post(r: r != 0) = func_ptr_target; // expected-error {{contracts may not be specified on a function pointer type}} \
                                                     // expected-error {{expected ';' after top level declarator}}

// === §3.3.7: Type alias with specifiers (ill-formed) ===

using ft = int(int) post(r: r != 0); // expected-error {{contracts may not be specified on a type alias}} \
                                      // expected-error {{expected ';' after alias declaration}}

// === §3.3.1: Lambda in redeclaration ===

void lambda_redecl() pre([]{ return true; }());
void lambda_redecl() pre([]{ return true; }());

// === §3.4.1: Incomplete type in predicate (ill-formed) ===

struct Incomplete; // expected-note {{forward declaration of 'Incomplete'}}
int incomplete_access(Incomplete* p) pre(p->x > 0); // expected-error {{member access into incomplete type 'Incomplete'}}

// === §3.2.1: Multiple specifiers in mixed order ===

void mixed_order()
  pre(true)
  post(true)
  pre(true); // OK

// === §3.4.1: Result binding shadows outer names ===

int r_shadow = 10;
int shadow_test() post(r_shadow: r_shadow != ::r_shadow); // OK

// === §3.5.7: Evaluation order of preconditions ===

void eval_order(int* p)
  pre(p != nullptr)
  pre(*p > 0); // OK

// === §3.4.3: Postcondition with deduced return type ===

auto deduced_with_body() post(r: r > 0) { // OK
  return 5;
}

auto deduced_no_result() post(true); // OK

// === §3.4.2: Lambda captures in contract predicates ===

void lambda_capture_test(int x)
  pre([x] { return x > 0; }()); // OK

// === Features NOT YET implemented (P2900R14 requirements) ===

// TODO §3.4.4: Parameters used in postcondition must be const
// TODO §3.4.8: Implicit capture restrictions in contract assertions
// TODO §3.4.2: Implicit const-ness of predicates
// TODO §3.4.1: this in member function preconditions
// TODO §3.3.4: Constructor direct member access restrictions
// TODO §3.3.5: Coroutine await/yield in predicates
