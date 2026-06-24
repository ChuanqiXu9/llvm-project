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
                                                              // expected-note@26 {{'local_j' declared here}} \
                                                              // expected-note@37 {{lambda expression begins here}} \
                                                              // expected-note@37 {{capture 'local_j' by value}} \
                                                              // expected-note@37 {{capture 'local_j' by reference}}

  // Multiple variables in predicate
  auto f9 = [=] pre(local_i + local_j > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
  auto f10 = [local_i, local_j] pre(local_i + local_j > 0) {};  // OK

  // Post-conditions also checked
  auto f11 = [=] post(r: local_i > 0) { return 0; };  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
  auto f12 = [local_i] post(r: local_i > 0) { return 0; };  // OK

  // Lambda without default capture - must explicitly capture
  auto f13 = [] pre(local_i > 0) {};  // expected-error {{variable 'local_i' cannot be implicitly captured in a lambda with no capture-default specified}} \
                                       // expected-note@25 {{'local_i' declared here}} \
                                       // expected-note@52 {{lambda expression begins here}} \
                                       // expected-note@52 {{capture 'local_i' by value}} \
                                       // expected-note@52 {{capture 'local_i' by reference}} \
                                       // expected-note@52 {{default capture by value}} \
                                       // expected-note@52 {{default capture by reference}}

  // Nested lambdas
  auto f14 = [=] {
    auto inner = [=] pre(local_i > 0) {};  // expected-error {{contract predicate cannot implicitly capture 'local_i'; explicitly capture it in the lambda}}
    auto inner2 = [local_i] pre(local_i > 0) {};  // OK
  };

  // Init captures
  auto f15 = [x = local_i] pre(x > 0) {};  // OK, x is explicitly captured

  // This capture
  struct S {
    int member;
    void method() {
      auto f = [this] pre(member > 0) {};  // OK
      auto f2 = [*this] pre(member > 0) {}; // OK
    }
  };
}
