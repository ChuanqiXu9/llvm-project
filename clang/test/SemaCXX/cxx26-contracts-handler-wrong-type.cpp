// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// Test: Wrong field type (_M_file is int instead of const char*)
namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  class contract_violation {
    int _M_file;  // Wrong type
    const char* _M_function;
    const char* _M_comment;
    unsigned int _M_line;
    contract_kind _M_kind;
    detection_mode_t _M_detection_mode;
  };
  void handle_contract_violation(const contract_violation&);
}

int f1(int x) pre(x > 0); // expected-error {{cannot use contract assertions: class 'std::contracts::contract_violation' does not have the expected field layout; include <contracts> to use contract assertions}}
