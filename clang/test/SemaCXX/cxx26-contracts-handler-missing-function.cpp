// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// The implementation-provided default handler has no user-visible
// declaration. A contract must therefore be accepted without one.
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
  enum class evaluation_semantic : unsigned short { ignore = 1, observe = 2, enforce = 3, quick_enforce = 4 };
  enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };
  class contract_violation;
}

int f1(int x) pre(x > 0);

// Building the contract must not introduce a declaration visible to the
// program.
auto *handler = &handle_contract_violation; // expected-error {{use of undeclared identifier 'handle_contract_violation'}}
