// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

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
