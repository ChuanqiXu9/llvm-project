// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// Test: Missing handle_contract_violation
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
}

int f1(int x) pre(x > 0); // expected-error {{cannot use contract assertions: function 'std::contracts::handle_contract_violation' not found; include <contracts> to use contract assertions}}
