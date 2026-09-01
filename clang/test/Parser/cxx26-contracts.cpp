// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

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

void handle_contract_violation(
    const std::contracts::contract_violation&);

int divide(int a, int b) pre(b != 0);

int square(int x) post(r: r >= 0);

int abs_val(int x) pre(x >= -1000) pre(x <= 1000) post(r: r >= 0);

int safe_div(const int a, const int b) pre(b != 0) post(r: r * b == a);

template <typename>
concept ContractConstrained = true;

template <typename T>
int constrained_decl(T value)
  requires ContractConstrained<T>
  pre(value > T{});

template <typename T>
auto constrained_definition(const T value) -> T
  requires ContractConstrained<T>
  pre(value > T{}) post(result: result >= value) {
  return value;
}

struct ConstrainedMember {
  template <typename T>
  auto f(const T value) -> T
    requires ContractConstrained<T>
    pre(value > T{}) post(result: result >= value) {
    return value;
  }
};

// Multiple post with result names
int clamp(int x) post(r: r >= 0) post(r: r <= 100);

// post without result name
int identity(const int x) post(x >= 0);

void f(int x) {
  contract_assert(x > 0);
}

void missing_contract_assert_lparen(int x) {
  contract_assert x > 0; // expected-error {{expected '(' after 'contract_assert'}}
}

// pre and post are not keywords - they can still be used as identifiers.
int pre = 42;
int post(int x) { return x; }
void use() {
  pre = 10;
  post(5);
}

// post(name: expr) on void-returning function is an error.
void g() post(r: r > 0); // expected-error {{post-condition result name on function returning void}} \
                          // expected-error {{use of undeclared identifier 'r'}}
