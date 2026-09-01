// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -ast-dump -ast-dump-filter=PartiallyInstantiatedPack %s | FileCheck %s --check-prefix=PARTIAL
// expected-no-diagnostics

// PARTIAL-LABEL: ClassTemplateSpecializationDecl {{.*}} PartiallyInstantiatedPack definition
// PARTIAL: CXXMethodDecl {{.*}} positive_sum 'int (const Args...)'
// PARTIAL: pre:
// PARTIAL-NEXT: {{.*}}CXXFoldExpr

namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
} // namespace std

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
  const char *_M_comment;
  const void *_M_src_loc_ptr;
  void *_M_ext;
};
} // namespace std::contracts

void handle_contract_violation(
    const std::contracts::contract_violation &);

struct Result {
  int *get() const;
  int use_count() const;
};

template <class T, class... Args>
Result make_result(Args &&...args)
  post(result: result.get() != nullptr && result.use_count() == 1);

template <class T, class... Args>
Result make_result(Args &&...args) {
  return {};
}

template <class... Args>
int sum(const int initial, const Args... args)
  pre(((args >= 0) && ...))
  post(result: result >= initial) {
  return initial + (args + ... + 0);
}

template <class... Args>
int const_ref_sum(const Args &...args)
  post(result: result >= (args + ... + 0)) {
  return (args + ... + 0);
}

template <class Callback>
void instantiate_from_dependent_context(Callback &&callback) {
  callback(make_result<int>());
  callback(make_result<int>(1, 2, 3));
  callback(sum(1));
  callback(sum(1, 2, 3));
  callback(const_ref_sum(1, 2, 3));
}

template <class T, class... Args>
Result make_result_in_lambda(Args &&...args)
  post(result: result.get() != nullptr && result.use_count() == 1) {
  return {};
}

template <class T, class... Args>
Result make_result_in_generic_lambda(Args &&...args)
  post(result: result.get() != nullptr && result.use_count() == 1) {
  return {};
}

template <class Callback>
using callback_result_t = decltype(((Callback *)nullptr)->operator()(0));

template <class Callback>
void inspect_callback(Callback) {
  using result_type = callback_result_t<Callback>;
}

template <class T>
struct PartiallyInstantiatedPack {
  template <class... Args>
  static int positive_sum(const Args... args)
    pre(((args > 0) && ...))
    post(result: result > 0) {
    return (args + ... + 0);
  }
};

template struct PartiallyInstantiatedPack<int>;

void use() {
  instantiate_from_dependent_context([](auto) {});

  [] {
    auto result = make_result_in_lambda<int>();
    (void)result;
  }();

  inspect_callback([](auto &&value) {
    return make_result_in_generic_lambda<int>((decltype(value) &&)value);
  });

  (void)PartiallyInstantiatedPack<int>::positive_sum(1, 2, 3);
}
