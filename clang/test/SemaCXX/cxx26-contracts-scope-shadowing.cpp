// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s
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

int value = 1;
int result = 2;

int global_shadow(int x) post(value: value > ::value) {
  return x + ::value;
}

int local_shadow(int x) {
  int result = x;
  auto lam = [result] post(result: result >= 0) { return result; };
  return lam();
}

int nested_lambda_shadow(const int x) post(result: result >= 0) {
  auto outer = [x] post(result: result >= x) {
    auto inner = [x] post(result: result >= x) { return x + 2; };
    return inner();
  };
  return outer();
}

struct MemberShadow {
  int value;
  int result;

  int by_this(int x) post(result: result > this->result) {
    return x + value;
  }

  int nested_member_lambda(int x) post(value: value > this->value) {
    auto lam = [this, x] post(value: value > this->value) {
      return x + this->value + this->result;
    };
    return lam();
  }
};

template <typename T>
T template_shadow(const T x) post(result: result >= x) {
  T result = x;
  return result;
}

template <typename T>
T template_nested_lambda_shadow(const T x) post(result: result >= x) {
  auto lam = [x] post(result: result >= x) { return x; };
  return lam();
}

namespace shadow_namespace {
int value = 3;
int result = 4;

int namespace_shadow(int x) post(value: value > shadow_namespace::value) {
  return x + shadow_namespace::value;
}

int namespace_result_shadow() post(result: result > shadow_namespace::result) {
  return shadow_namespace::result + 1;
}
}

int block_scope_result_shadow(int x) {
  int result = x;
  {
    int value = result;
    auto lam = [value] post(result: result >= value) { return value + 1; };
    result = lam();
  }
  return result;
}

void instantiate_shadow_tests() {
  global_shadow(1);
  local_shadow(1);
  nested_lambda_shadow(1);
  MemberShadow{1, 0}.by_this(1);
  MemberShadow{1, 0}.nested_member_lambda(1);
  template_shadow(1);
  template_shadow(2L);
  template_nested_lambda_shadow(1);
  template_nested_lambda_shadow(2L);
  shadow_namespace::namespace_shadow(1);
  shadow_namespace::namespace_result_shadow();
  block_scope_result_shadow(1);
}
