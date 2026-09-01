// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

bool predicate();

// CHECK-LABEL: define {{.*}} @_Z1fv(
// CHECK-NOT: call {{.*}}predicate
// CHECK-NOT: contract.handler
// CHECK-NOT: handle_contract_violation
// CHECK-NOT: abort
// CHECK: ret void
void f() pre(predicate()) {
  contract_assert(predicate());
}

// CHECK-LABEL: define {{.*}} @_Z1gv(
// CHECK-NOT: call {{.*}}predicate
// CHECK-NOT: contract.handler
// CHECK-NOT: handle_contract_violation
// CHECK-NOT: abort
// CHECK: ret i32 0
int g() post(predicate()) {
  return 0;
}