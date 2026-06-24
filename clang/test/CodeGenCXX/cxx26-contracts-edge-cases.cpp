// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -emit-llvm -o - %s | FileCheck %s

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

// === 1. Static member functions ===

struct StaticHolder {
  static int add(int a, int b) pre(a > 0) pre(b > 0) post(r: r > 0) {
    return a + b;
  }
};

// === 2. Functions in namespaces ===

namespace outer {
  namespace inner {
    int process(int x) pre(x > 0) post(r: r > 0) {
      return x * 2;
    }
  }
}

// === 3. [[noreturn]] functions ===

[[noreturn]] void fatal_error(int code) pre(code != 0) {
  __builtin_abort();
}

// === 4. noexcept functions ===

int safe_divide(int a, int b) noexcept pre(b != 0) {
  return a / b;
}

// === 5. Deep inheritance chain ===

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

// === 6. Functions with default arguments ===

int with_defaults(int a, int b = 10) pre(a > 0) pre(b > 0) post(r: r > 0) {
  return a + b;
}

// === 7. Template with contract_assert ===

template<typename T>
T max_val(T a, T b) {
  contract_assert(a == a);
  contract_assert(b == b);
  return a > b ? a : b;
}

void run_tests() {
  StaticHolder::add(1, 2);
  outer::inner::process(5);
  safe_divide(10, 2);

  Base base;
  base.process(5);

  Mid1 mid1;
  mid1.transform(5);

  Mid2 mid2;
  mid2.compute(5);

  Leaf leaf;
  leaf.final_step(5);

  with_defaults(5);
  with_defaults(5, 20);

  max_val(10, 20);
}

// Verify all functions are generated with contract checks (order-independent)
// CHECK-DAG: define {{.*}} @_ZN12StaticHolder3addEii
// CHECK-DAG: define {{.*}} @_ZN5outer5inner7processEi
// CHECK-DAG: define {{.*}} @_Z11fatal_errori
// CHECK-DAG: define {{.*}} @_Z11safe_divideii
// CHECK-DAG: define {{.*}} @_ZN4Base7processEi
// CHECK-DAG: define {{.*}} @_ZN4Mid19transformEi
// CHECK-DAG: define {{.*}} @_ZN4Mid27computeEi
// CHECK-DAG: define {{.*}} @_ZN4Leaf10final_stepEi
// CHECK-DAG: define {{.*}} @_Z13with_defaultsii
// CHECK-DAG: define {{.*}} @_Z7max_valIiET_S0_S0_
// CHECK-DAG: icmp sgt i32
// CHECK-DAG: icmp ne i32
// CHECK-DAG: icmp eq i32
