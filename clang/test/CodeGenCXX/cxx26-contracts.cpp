// RUN: %clang_cc1 -std=c++2c -fcontracts -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=ENFORCE
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=observe -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=IGNORE
// RUN: %clang_cc1 -std=c++2c -fcontracts -triple x86_64-pc-windows-msvc -emit-llvm -o - %s | FileCheck %s --check-prefix=MS

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
  enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };
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
// --- pre-condition ---

int divide(int a, int b) pre(b != 0) {
  return a / b;
}

// ENFORCE-LABEL: define {{.*}} @_Z6divideii(
// ENFORCE:   %[[B:.*]] = load i32, ptr %b.addr
// ENFORCE:   %[[CMP:.*]] = icmp ne i32 %[[B]], 0
// ENFORCE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   %[[DIV_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// ENFORCE-NEXT: store i16 1, ptr %[[DIV_KIND]]
// ENFORCE:   %[[DIV_SEMANTIC:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 2
// ENFORCE-NEXT: store i16 3, ptr %[[DIV_SEMANTIC]]
// ENFORCE:   %[[DIV_MODE:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 3
// ENFORCE-NEXT: store i16 1, ptr %[[DIV_MODE]]
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// MS:        call void @"?handle_contract_violation@@YAXAEBVcontract_violation@contracts@std@@@Z"(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z6divideii(
// OBSERVE:   %[[CMP:.*]] = icmp ne i32 %{{.*}}, 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   %[[DIV_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// OBSERVE-NEXT: store i16 1, ptr %[[DIV_KIND]]
// OBSERVE:   %[[DIV_SEMANTIC:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 2
// OBSERVE-NEXT: store i16 2, ptr %[[DIV_SEMANTIC]]
// OBSERVE:   %[[DIV_MODE:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 3
// OBSERVE-NEXT: store i16 1, ptr %[[DIV_MODE]]
// OBSERVE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// OBSERVE:   br label %contract.cont
// OBSERVE: contract.cont:

// IGNORE-LABEL: define {{.*}} @_Z6divideii(
// IGNORE-NOT: @abort
// IGNORE-NOT: handle_contract_violation
// IGNORE:   ret i32

// --- post-condition with result name ---

int square(int x) post(r: r >= 0) {
  return x * x;
}

// ENFORCE-LABEL: define {{.*}} @_Z6squarei(
// ENFORCE:   store i32 %mul, ptr %retval
// ENFORCE:   %[[RET:.*]] = load i32, ptr %retval
// ENFORCE:   %[[CMP:.*]] = icmp sge i32 %[[RET]], 0
// ENFORCE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   %[[SQUARE_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// ENFORCE-NEXT: store i16 2, ptr %[[SQUARE_KIND]]
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z6squarei(
// OBSERVE:   store i32 %mul, ptr %retval
// OBSERVE:   %[[RET:.*]] = load i32, ptr %retval
// OBSERVE:   %[[CMP:.*]] = icmp sge i32 %[[RET]], 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   %[[SQUARE_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// OBSERVE-NEXT: store i16 2, ptr %[[SQUARE_KIND]]
// OBSERVE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(

// IGNORE-LABEL: define {{.*}} @_Z6squarei(
// IGNORE-NOT: @abort
// IGNORE-NOT: handle_contract_violation
// IGNORE:   ret i32

// --- post-condition without result name ---

int identity(const int x) post(x >= 0) {
  return x;
}

// ENFORCE-LABEL: define {{.*}} @_Z8identityi(
// ENFORCE:   icmp sge i32 %{{.*}}, 0
// ENFORCE:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()

// --- contract_assert ---

void check(int x) {
  contract_assert(x > 0);
}

// ENFORCE-LABEL: define {{.*}} @_Z5checki(
// ENFORCE:   %[[X:.*]] = load i32, ptr %x.addr
// ENFORCE:   %[[CMP:.*]] = icmp sgt i32 %[[X]], 0
// ENFORCE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   %[[ASSERT_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// ENFORCE-NEXT: store i16 3, ptr %[[ASSERT_KIND]]
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z5checki(
// OBSERVE:   %[[CMP:.*]] = icmp sgt i32 %{{.*}}, 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   %[[ASSERT_KIND:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 1
// OBSERVE-NEXT: store i16 3, ptr %[[ASSERT_KIND]]
// OBSERVE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(

// IGNORE-LABEL: define {{.*}} @_Z5checki(
// IGNORE-NOT: @abort
// IGNORE-NOT: handle_contract_violation
// IGNORE:   ret void

// --- multiple pre and post ---

int clamp(int x) pre(x >= -1000) pre(x <= 1000) post(r: r >= 0) post(r: r <= 1000) {
  return x < 0 ? -x : x;
}

// ENFORCE-LABEL: define {{.*}} @_Z5clampi(
// ENFORCE:   %[[CMP1:.*]] = icmp sge i32 %{{.*}}, -1000
// ENFORCE:   br i1 %[[CMP1]], label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE:   %[[CMP2:.*]] = icmp sle i32 %{{.*}}, 1000
// ENFORCE:   br i1 %[[CMP2]], label %contract.cont{{.*}}, label %contract.handler{{.*}}

// --- contract inheritance on redeclaration ---

int safe_div(int a, int b) pre(b != 0);
int safe_div(int a, int b) {
  return a / b;
}

// ENFORCE-LABEL: define {{.*}} @_Z8safe_divii(
// ENFORCE:   icmp ne i32 %{{.*}}, 0
// ENFORCE:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()

// OBSERVE-LABEL: define {{.*}} @_Z8safe_divii(
// OBSERVE:   icmp ne i32 %{{.*}}, 0
// OBSERVE:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(

// IGNORE-LABEL: define {{.*}} @_Z8safe_divii(
// IGNORE-NOT: @abort
// IGNORE-NOT: handle_contract_violation
// IGNORE:   ret i32

// --- void function with pre and contract_assert ---

void validate(int x) pre(x != 0) {
  contract_assert(x > -100);
}

// ENFORCE-LABEL: define {{.*}} @_Z8validatei(
// ENFORCE:   icmp ne i32 %{{.*}}, 0
// ENFORCE:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE:   icmp sgt i32 %{{.*}}, -100
// ENFORCE:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()

// --- verify synthesized violation object materialization ---
// Check that the frontend materializes a layout-compatible temporary object.

int guarded(int x) pre(x > 0) {
  return x;
}

// ENFORCE-LABEL: define {{.*}} @_Z7guardedi(
// ENFORCE: contract.handler:
// ENFORCE:   getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 0
// ENFORCE-NEXT: store i16 1,
// ENFORCE:   call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// ENFORCE:   call void @abort()
