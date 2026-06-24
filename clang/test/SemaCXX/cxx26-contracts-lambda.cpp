// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

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
  void handle_contract_violation(const contract_violation&);
}

void test() {
  // Test 1: Lambda with pre-condition (works with deduced return type)
  auto lam1 = [](int x) pre(x > 0) { return x; };

  // Test 2: Lambda with post-condition with result name and deduced return type
  auto lam2 = [](const int x) post(r: r >= 0) { return x * x; };

  // Test 3: Lambda with trailing return type and post-condition (works)
  auto lam3 = [](const int x) -> int post(r: r > 0) { return x * 2; };

  // Test 4: Lambda with both pre and post with explicit return type (works)
  auto lam4 = [](const int x) -> int pre(x > 0) post(r: r > 0) { return x + 1; };

  // Test 5: Lambda with both pre and post with deduced return type
  auto lam5 = [](const int x) pre(x > 0) post(r: r > 0) { return x + 1; };

  // Test 6: Lambda with multiple post-conditions with explicit return type (works)
  auto lam6 = [](const int x) -> int post(r: r >= 0) post(r: r <= 100) { return x % 100; };

  // Test 7: Lambda with post-condition without result name (works)
  auto lam7 = [](const int x) post(x >= 0) { return x; };
}
