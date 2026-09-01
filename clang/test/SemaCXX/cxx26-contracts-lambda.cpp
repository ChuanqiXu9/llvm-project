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

template <class> struct IsConstCall;
template <class C, class R, class... Args>
struct IsConstCall<R (C::*)(Args...) const> {
  static constexpr bool value = true;
};
template <class C, class R, class... Args>
struct IsConstCall<R (C::*)(Args...)> {
  static constexpr bool value = false;
};
template <class C, class R, class... Args>
struct IsConstCall<R (C::*)(Args...) const noexcept> {
  static constexpr bool value = true;
};
template <class C, class R, class... Args>
struct IsConstCall<R (C::*)(Args...) noexcept> {
  static constexpr bool value = false;
};

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

  // Test 8: Parenthesized non-mutable lambdas with contracts still get a const
  // call operator. This catches regressions where parsing contracts skips
  // ActOnLambdaClosureQualifiers.
  auto lam8 = [](int x) pre(x > 0) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam8)::operator())>::value);

  // Test 9: Mutable lambdas with contracts still get a non-const call operator.
  auto lam9 = [](const int x) mutable pre(x > 0) post(r: r >= x) { return x; };
  static_assert(!IsConstCall<decltype(&decltype(lam9)::operator())>::value);

  // Test 10: Mutable lambdas with trailing return types and contracts.
  auto lam10 = [](const int x) mutable -> int pre(x > 0) post(r: r >= x) { return x; };
  static_assert(!IsConstCall<decltype(&decltype(lam10)::operator())>::value);

  // Test 11: Parentheses-less lambdas with contracts keep the const call operator.
  auto lam11 = [] pre(true) { return 0; };
  static_assert(IsConstCall<decltype(&decltype(lam11)::operator())>::value);

  // Test 12: Generic lambdas with contracts and deduced return type.
  auto lam12 = []<typename T>(T x) pre(x > T{}) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam12)::template operator()<int>)>::value);
  lam12(1);

  // Test 13: Generic lambdas with post-condition result names and deduced return type.
  auto lam13 = []<typename T>(const T x) post(r: r >= x) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam13)::template operator()<int>)>::value);
  lam13(1);

  // Test 13b: Lambda with noexcept and contracts.
  auto lam13b = [](const int x) noexcept pre(x > 0) post(r: r >= x) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam13b)::operator())>::value);
  lam13b(1);

  // Test 13c: Generic lambda with explicit trailing return type and contracts.
  auto lam13c = []<typename T>(const T x) -> T pre(x >= T{}) post(r: r >= x) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam13c)::template operator()<int>)>::value);
  lam13c(1);

  // Test 14: Generic lambdas with multiple post-condition result names.
  auto lam14 = []<typename T>(const T x) post(r: r >= T{}) post(result: result >= x) {
    return x;
  };
  static_assert(IsConstCall<decltype(&decltype(lam14)::template operator()<int>)>::value);
  lam14(1);

  // Test 14b: Lambda with default argument and contracts.
  auto lam14b = [](int x = 1) pre(x > 0) { return x; };
  static_assert(IsConstCall<decltype(&decltype(lam14b)::operator())>::value);
  lam14b();
  lam14b(2);

  // Test 14c: Generic lambda with a non-type template parameter.
  auto lam14c = []<int N>() pre(N > 0) post(r: r == N) { return N; };
  static_assert(IsConstCall<decltype(&decltype(lam14c)::template operator()<1>)>::value);
  lam14c.template operator()<1>();

  // Test 14d: Lambda returning by reference with a result-name postcondition.
  static int storage = 1;
  auto lam14d = []() -> int & pre(storage > 0) post(r: r > 0) { return storage; };
  static_assert(IsConstCall<decltype(&decltype(lam14d)::operator())>::value);
  lam14d();
}

template <typename T>
void test_generic_lambda_in_template(T x) {
  // Test 15: Generic lambda post-condition result variables are rebuilt after
  // return type deduction when the lambda is instantiated from a dependent
  // context.
  auto lam = []<typename U>(const U y) post(r: r >= y) { return y; };
  static_assert(IsConstCall<decltype(&decltype(lam)::template operator()<T>)>::value);
  lam(x);
}

void instantiate_template_context() {
  test_generic_lambda_in_template(1);
  test_generic_lambda_in_template(2L);
}

template <typename T>
void test_nested_generic_lambda_template(T x) {
  // Test 16: Nested generic lambdas keep independent post-condition result
  // variables through template instantiation.
  auto outer = []<typename U>(const U y) post(r: r >= y) {
    auto inner = []<typename V>(const V z) post(r: r >= z) { return z; };
    return inner(y);
  };
  outer(x);
}

void instantiate_nested_template_context() {
  test_nested_generic_lambda_template(1);
  test_nested_generic_lambda_template(2L);
}

template <int N>
void test_lambda_with_outer_nttp() {
  auto lam = []<int M>() pre(M > 0) post(r: r == M) { return M; };
  lam.template operator()<N>();
}

void instantiate_lambda_with_outer_nttp() {
  test_lambda_with_outer_nttp<1>();
  test_lambda_with_outer_nttp<2>();
}

template <typename>
concept LambdaContractConstrained = true;

void test_constrained_lambdas() {
  // Test 17: The trailing requires-clause precedes contract specifiers.
  auto constrained = []<typename T>(T x)
    requires LambdaContractConstrained<T>
    pre(x > T{}) {
    return x;
  };
  constrained(1);

  // A template-head requires-clause and a trailing requires-clause can both
  // appear on the same lambda; contracts follow the latter.
  auto doubly_constrained = []<typename T>
    requires LambdaContractConstrained<T>
    (const T x) -> T
    requires LambdaContractConstrained<T>
    pre(x > T{}) post(result: result >= x) {
    return x;
  };
  doubly_constrained(1);
}
