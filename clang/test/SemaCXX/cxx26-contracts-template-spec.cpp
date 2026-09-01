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
}
void handle_contract_violation(const std::contracts::contract_violation&);

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

template <int N>
int nttp_value() pre(N >= 0) post(r: r >= 0) {
  return N;
}

template <>
int nttp_value<0>() post(r: r == 0) {
  return 0;
}

template <typename T>
struct SpecHolder {
  static int get(T x) pre(x > T{}) post(r: r >= 0) {
    return static_cast<int>(x);
  }
};

template <>
struct SpecHolder<int> {
  static int get(const int x) pre(x >= 0) post(r: r >= x) {
    return x;
  }
};

void test() {
  foo(5);              // primary template
  foo<int>(5);         // specialization without contracts
  foo<double>(5.0);    // specialization with its own contracts
  foo<float>(5.0f);    // specialization with different contracts
  nttp_value<0>();
  nttp_value<5>();
  SpecHolder<double>::get(2.0);
  SpecHolder<int>::get(2);
}
