// RUN: %clang_cc1 -std=c++2c -fcontracts -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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

// Test 1: Function template with contract_assert
template<typename T>
void check_positive(T x) {
  contract_assert(x > 0);
}

// CHECK: define {{.*}} @_Z14check_positiveIiEvT_(
// CHECK: %cmp = icmp sgt i32 %{{.*}}, 0
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()

// CHECK: define {{.*}} @_Z14check_positiveIdEvT_(
// CHECK: %cmp = fcmp ogt double %{{.*}}, 0.000000e+00
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()

// Test 2: Class template member function with contract_assert
template<typename T>
class Container {
public:
  void check_size(int size) {
    contract_assert(size > 0);
  }
};

// CHECK: define {{.*}} @_ZN9ContainerIiE10check_sizeEi(
// CHECK: %cmp = icmp sgt i32 %{{.*}}, 0
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()

// CHECK: define {{.*}} @_ZN9ContainerIdE10check_sizeEi(
// CHECK: %cmp = icmp sgt i32 %{{.*}}, 0
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()

void test() {
  check_positive(5);
  check_positive(3.14);

  Container<int> c1;
  c1.check_size(50);

  Container<double> c2;
  c2.check_size(100);
}
