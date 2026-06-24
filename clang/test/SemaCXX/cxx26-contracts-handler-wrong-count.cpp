// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

// Test: Missing evaluation_semantic
namespace std::contracts {
  struct source_location {
    struct __impl {
      const char* _M_file_name;
      const char* _M_function_name;
      unsigned _M_line;
      unsigned _M_column;
    };
  };
  enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
  enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };
  class contract_violation {
    unsigned short _M_version;
    assertion_kind _M_assertion_kind;
    int _M_evaluation_semantic;
    detection_mode _M_detection_mode;
    const char* _M_comment;
    const void* _M_src_loc_ptr;
    void* _M_ext;
  };
  void handle_contract_violation(const contract_violation&);
}

int f1(int x) pre(x > 0); // expected-error {{cannot use contract assertions: enum 'std::contracts::evaluation_semantic' not found; include <contracts> to use contract assertions}}
