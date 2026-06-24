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

template<typename T>
int foo(T x) pre(x > 0) post(r: r > 0) {
  return x;
}

// Explicit specialization without contracts - should be allowed
template<>
int foo<int>(int x) {
  return x * 2;
}

// Explicit specialization with its own contracts - should be allowed
template<>
int foo<double>(double x) pre(x > 1.0) {
  return x * 3;
}

// Explicit specialization with different contracts - should be allowed
template<>
int foo<float>(float x) pre(x > 0.0f) post(r: r >= 0) {
  return x * 4;
}

void test() {
  foo(5);              // primary template
  foo<int>(5);         // specialization without contracts
  foo<double>(5.0);    // specialization with its own contracts
  foo<float>(5.0f);    // specialization with different contracts
}
