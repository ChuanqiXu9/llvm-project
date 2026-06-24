// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// === 1. Missing contract_violation ===
namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  void handle_contract_violation(int);
}
int f1(int x) pre(x > 0); // expected-error {{cannot use contract assertions: class 'std::contracts::contract_violation' not found; include <contracts> to use contract assertions}}
