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

// P2900R14 §3.4.8: Lambda contract predicates cannot implicitly capture
// variables from the enclosing scope.

static int static_i = 0;
int global_i = 0;

void test() {
  // Globals and statics don't need to be captured
  auto f1 = [=] pre(global_i > 0) {};  // OK
  auto f2 = [=] pre(static_i > 0) {};  // OK
  auto f3 = [] pre(global_i > 0) {};   // OK

  int local_i = 1;
  int local_j = 2;

  // Implicit captures are not allowed in contract predicates
  auto f4 = [=] pre(local_i > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
  auto f5 = [&] pre(local_i > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}

  // Explicit captures are OK
  auto f6 = [local_i] pre(local_i > 0) {};  // OK
  auto f7 = [&local_i] pre(local_i > 0) {}; // OK

  // Mixed: explicit capture of one, implicit use of another
  auto f8 = [local_i] pre(local_i > 0 && local_j > 0) {};  // expected-error {{variable 'local_j' cannot be implicitly captured in a lambda with no capture-default specified}} \
                                                              // expected-note@51 {{'local_j' declared here}} \
                                                              // expected-note@62 {{lambda expression begins here}} \
                                                              // expected-note@62 {{capture 'local_j' by value}} \
                                                              // expected-note@62 {{capture 'local_j' by reference}}

  // Multiple variables in predicate
  auto f9 = [=] pre(local_i + local_j > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
  auto f10 = [local_i, local_j] pre(local_i + local_j > 0) {};  // OK

  // Post-conditions also checked
  auto f11 = [=] post(r: local_i > 0) { return 0; };  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
  auto f12 = [local_i] post(r: local_i > 0) { return 0; };  // OK

  // Lambda without default capture - must explicitly capture
  auto f13 = [] pre(local_i > 0) {};  // expected-error {{variable 'local_i' cannot be implicitly captured in a lambda with no capture-default specified}} \
                                       // expected-note@50 {{'local_i' declared here}} \
                                       // expected-note@77 {{lambda expression begins here}} \
                                       // expected-note@77 {{capture 'local_i' by value}} \
                                       // expected-note@77 {{capture 'local_i' by reference}} \
                                       // expected-note@77 {{default capture by value}} \
                                       // expected-note@77 {{default capture by reference}}

  // Nested lambdas
  auto f14 = [=] {
    auto inner = [=] pre(local_i > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
    auto inner2 = [local_i] pre(local_i > 0) {};  // OK
  };

  // Init captures
  auto f15 = [x = local_i] pre(x > 0) {};  // OK, x is explicitly captured
  auto f16 = [x = local_i] post(r: x > 0) { return x; };  // OK, x is explicitly captured

  // Explicit captures in post-conditions are OK.
  auto f17 = [local_i] post(r: local_i > 0) { return local_i; };
  auto f18 = [&local_i] post(r: local_i > 0) { return local_i; };
  auto f19 = [local_i, local_j] post(r: r >= local_i + local_j) {
    return local_i + local_j;
  };

  // Mutable lambdas with explicit captures still do not require implicit capture.
  auto f20 = [local_i] mutable pre(local_i > 0) post(r: r >= local_i) {
    return local_i;
  };

  // Nested post-condition predicates follow the same explicit-capture rule.
  auto f21 = [local_i] {
    auto inner = [local_i] post(r: r >= local_i) { return local_i; };
    return inner();
  };

  // This capture
  struct S {
    int member;
    void method() {
      auto f = [this] pre(member > 0) {};  // OK
      auto f2 = [*this] pre(member > 0) {}; // OK
    }
  };
}
