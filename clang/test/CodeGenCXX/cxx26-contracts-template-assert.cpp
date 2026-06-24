// RUN: %clang_cc1 -std=c++2c -fcontracts -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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

// Test 1: Function template with contract_assert
template<typename T>
void check_positive(T x) {
  contract_assert(x > 0);
}

// CHECK: define {{.*}} @_Z14check_positiveIiEvT_(
// CHECK: %cmp = icmp sgt i32 %{{.*}}, 0
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()

// CHECK: define {{.*}} @_Z14check_positiveIdEvT_(
// CHECK: %cmp = fcmp ogt double %{{.*}}, 0.000000e+00
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
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
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()

// CHECK: define {{.*}} @_ZN9ContainerIdE10check_sizeEi(
// CHECK: %cmp = icmp sgt i32 %{{.*}}, 0
// CHECK: br i1 %cmp, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()

void test() {
  check_positive(5);
  check_positive(3.14);

  Container<int> c1;
  c1.check_size(50);

  Container<double> c2;
  c2.check_size(100);
}
