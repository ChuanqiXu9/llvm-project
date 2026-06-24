// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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

struct S {
  int value;
  S(const int v) pre(v > 0) post(value == v) : value(v) {}
  ~S() pre(value > 0) post(true) {}
};

void use() {
  S s(1);
}

// CHECK-LABEL: define {{.*}} @_ZN1SC2Ei(
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()
// CHECK: contract.handler{{.*}}:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()

// CHECK-LABEL: define {{.*}} @_ZN1SD2Ev(
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// CHECK: call void @abort()
// CHECK: contract.handler{{.*}}:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
