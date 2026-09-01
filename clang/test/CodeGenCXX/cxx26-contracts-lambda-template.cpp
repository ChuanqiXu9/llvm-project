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

void use(int);

// --- Lambda with pre-condition ---

void test_lambda_pre() {
  auto lam = [](int x) pre(x > 0) { return x; };
  use(lam(5));
}

// CHECK-LABEL: define {{.*}} @_Z15test_lambda_prev()
// CHECK: define {{.*}} @"_ZZ15test_lambda_prevENK3$_0clEi"(
// CHECK:   %[[CMP:.*]] = icmp sgt i32 %{{.*}}, 0
// CHECK:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK:   call void @abort()
// CHECK-NEXT: unreachable
// CHECK: contract.cont:

// --- Lambda with explicit return type and post-condition ---

void test_lambda_post() {
  auto lam = [](int x) -> int post(r: r >= 0) { return x * x; };
  use(lam(5));
}

// CHECK-LABEL: define {{.*}} @_Z16test_lambda_postv()
// CHECK: define {{.*}} @"_ZZ16test_lambda_postvENK3$_0clEi"(
// CHECK:   store i32 %mul, ptr %retval
// CHECK:   %[[RET:.*]] = load i32, ptr %retval
// CHECK:   %[[CMP:.*]] = icmp sge i32 %[[RET]], 0
// CHECK:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK:   call void @abort()
// CHECK-NEXT: unreachable
// CHECK: contract.cont:

// --- Lambda with both pre and post ---

void test_lambda_both() {
  auto lam = [](int x) -> int pre(x > 0) post(r: r > 0) { return x * 2; };
  use(lam(5));
}

// CHECK-LABEL: define {{.*}} @_Z16test_lambda_bothv()
// CHECK: define {{.*}} @"_ZZ16test_lambda_bothvENK3$_0clEi"(
// CHECK:   %[[CMP1:.*]] = icmp sgt i32 %{{.*}}, 0
// CHECK:   br i1 %[[CMP1]], label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK:   call void @abort()
// CHECK: contract.cont:
// CHECK:   store i32 %mul, ptr %retval
// CHECK:   %[[RET:.*]] = load i32, ptr %retval
// CHECK:   %[[CMP2:.*]] = icmp sgt i32 %[[RET]], 0
// CHECK:   br i1 %[[CMP2]], label %contract.cont{{.*}}, label %contract.handler{{.*}}

// --- Generic lambda with deduced return type and post-condition result name ---

void test_generic_lambda_post() {
  auto lam = []<typename T>(const T x) post(r: r >= x) { return x; };
  use(lam(5));
}

// CHECK-LABEL: define {{.*}} @_Z24test_generic_lambda_postv()
// CHECK: define {{.*}} @"_ZZ24test_generic_lambda_postvENK3$_0clIiEEDaT_"(
// CHECK:   store i32 %{{.*}}, ptr %retval
// CHECK:   %[[GEN_RET:.*]] = load i32, ptr %retval
// CHECK:   %[[GEN_CMP:.*]] = icmp sge i32 %[[GEN_RET]], %{{.*}}
// CHECK:   br i1 %[[GEN_CMP]], label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK:   call void @abort()
