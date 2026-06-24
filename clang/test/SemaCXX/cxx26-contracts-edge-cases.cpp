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
  void handle_contract_violation(const contract_violation&);
}

// === 1. contract_assert with dependent type in template ===

template<typename T>
void check_positive(T x) {
  contract_assert(x > T(0));
}

template<typename T>
int compute(T x) {
  contract_assert(x != T());
  return static_cast<int>(x);
}

// Template with multiple contract_assert using different dependent types
template<typename T, typename U>
void check_both(T a, U b) {
  contract_assert(a > T(0));
  contract_assert(b > U(0));
}

// === 2. Static member functions ===

struct StaticHolder {
  static int add(int a, int b) pre(a > 0) pre(b > 0) post(r: r > 0) {
    return a + b;
  }

  static void validate(int x) pre(x >= 0) {
    contract_assert(x < 1000);
  }
};

// === 3. Functions in namespaces ===

namespace outer {
  namespace inner {
    int process(int x) pre(x > 0) post(r: r > 0) {
      return x * 2;
    }

    void check(int x) pre(x != 0) {
      contract_assert(x > -1000);
    }
  }
}

// === 4. [[noreturn]] functions ===

[[noreturn]] void fatal_error(int code) pre(code != 0) {
  contract_assert(code > 0);
  __builtin_abort();
}

// === 5. noexcept functions ===

int safe_divide(int a, int b) noexcept pre(b != 0) {
  return a / b;
}

void noexcept_check(int x) noexcept pre(x > 0) {
  contract_assert(x < 100);
}

// === 6. Deep inheritance chain with non-virtual member functions ===

struct Base {
  int process(int x) pre(x > 0) {
    return x;
  }
};

struct Mid1 : Base {
  int transform(int x) pre(x > 0) post(r: r > 0) {
    return x * 2;
  }
};

struct Mid2 : Mid1 {
  int compute(int x) pre(x > 0) {
    return transform(x);
  }
};

struct Leaf : Mid2 {
  int final_step(int x) pre(x > 0) post(r: r > 0) {
    return compute(x) + 1;
  }
};

// === 7. Functions with default arguments ===

int with_defaults(int a, int b = 10) pre(a > 0) pre(b > 0) post(r: r > 0) {
  return a + b;
}

void check_defaults(int x = 5, int y = 10) pre(x > 0) pre(y > 0) {
  contract_assert(x + y > 0);
}

// === 8. Mixed: static + default + noexcept ===

struct Mixed {
  static int compute(int a, int b = 0) noexcept pre(a >= 0) {
    return a + b;
  }
};

// === 9. Template with contract_assert on dependent comparison ===

template<typename T>
T max_val(T a, T b) {
  contract_assert(a == a);  // self-comparison, dependent type
  contract_assert(b == b);
  return a > b ? a : b;
}

void test_all() {
  // Dependent type templates
  check_positive(5);
  check_positive(3.14);
  compute(42);
  check_both(5, 3.14);
  max_val(10, 20);

  // Static members
  StaticHolder::add(1, 2);
  StaticHolder::validate(50);

  // Namespaced
  outer::inner::process(5);
  outer::inner::check(10);

  // noexcept
  safe_divide(10, 2);
  noexcept_check(5);

  // Non-virtual inheritance chain
  Leaf leaf;
  leaf.final_step(5);

  // Default arguments
  with_defaults(5);
  with_defaults(5, 20);
  check_defaults();
  check_defaults(3, 7);

  // Mixed
  Mixed::compute(5);
  Mixed::compute(5, 10);
}

// === 11. Trailing return type auto ===

auto trailing_auto(int x) -> auto pre(x > 0) post(r: r > 0) {
  return x * 2;
}

auto trailing_auto_complex(int x, int y) -> auto pre(x > 0) pre(y > 0) post(r: r >= 0) {
  return x + y;
}
