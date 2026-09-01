// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s
// Tests based on P2900R14 examples not covered by other test files.

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

// === §3.3.6: Function pointer with specifiers (ill-formed) ===

int func_ptr_target(int x) post(r: r != 0);
int (*fp1)(int) post(r: r != 0) = func_ptr_target; // expected-error {{contracts may not be specified on a function pointer type}}

// === §3.3.7: Type alias with specifiers (ill-formed) ===

using ft = int(int) post(r: r != 0); // expected-error {{contracts may not be specified on a type alias}}

// === §3.3.1: Lambda in redeclaration ===

void lambda_redecl() pre([]{ return true; }());
void lambda_redecl() pre([]{ return true; }()); // accepted: no diagnostic required

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

int nested_lambda_result_shadow() post(r: r > 0) {
  auto inner = [] post(r: r > 0) { return 1; };
  return inner();
}

struct MemberResultShadow {
  int r;
  int f() post(r: r > this->r) { return r + 1; }
};

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

struct Awaiter {
  bool await_ready();
  void await_suspend(int);
  int await_resume();
};

int coroutine_predicate_await()
  pre(co_await Awaiter{} > 0); // expected-error {{contract predicate cannot contain co_await}}

void coroutine_predicate_yield()
  post((co_yield 1, true)) {} // expected-error {{contract predicate cannot contain co_yield}}

void contract_assert_predicate_await() {
  contract_assert(co_await Awaiter{} > 0); // expected-error {{contract predicate cannot contain co_await}}
}

void contract_assert_predicate_yield() {
  contract_assert((co_yield 1, true)); // expected-error {{contract predicate cannot contain co_yield}}
}

auto deduced_multiple_returns(bool flag) post(r: r > 0) { if (flag) return 1; return 2; }
void void_with_postcondition() post(true) {}
void nested_lambda_capture_test(int x) pre(([x] { return [x] { return x > 0; }(); })());
