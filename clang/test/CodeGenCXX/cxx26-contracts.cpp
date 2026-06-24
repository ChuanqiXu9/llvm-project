// RUN: %clang_cc1 -std=c++2c -fcontracts -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=ENFORCE
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=observe -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=IGNORE

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

// --- pre-condition ---

int divide(int a, int b) pre(b != 0) {
  return a / b;
}

// ENFORCE-LABEL: define {{.*}} @_Z6divideii(
// ENFORCE:   %[[B:.*]] = load i32, ptr %b.addr
// ENFORCE:   %[[CMP:.*]] = icmp ne i32 %[[B]], 0
// ENFORCE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// ENFORCE: contract.handler:
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z6divideii(
// OBSERVE:   %[[CMP:.*]] = icmp ne i32 %{{.*}}, 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z6squarei(
// OBSERVE:   store i32 %mul, ptr %retval
// OBSERVE:   %[[RET:.*]] = load i32, ptr %retval
// OBSERVE:   %[[CMP:.*]] = icmp sge i32 %[[RET]], 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(

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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE-NEXT: unreachable
// ENFORCE: contract.cont:

// OBSERVE-LABEL: define {{.*}} @_Z5checki(
// OBSERVE:   %[[CMP:.*]] = icmp sgt i32 %{{.*}}, 0
// OBSERVE:   br i1 %[[CMP]], label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(

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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()

// OBSERVE-LABEL: define {{.*}} @_Z8safe_divii(
// OBSERVE:   icmp ne i32 %{{.*}}, 0
// OBSERVE:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// OBSERVE: contract.handler:
// OBSERVE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(

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
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()
// ENFORCE:   icmp sgt i32 %{{.*}}, -100
// ENFORCE:   br i1 %{{.*}}, label %{{.*}}, label %{{.*}}
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(
// ENFORCE:   call void @abort()

// --- verify violation struct fields for a specific function ---
// Check that the contract_violation struct is properly constructed.

int guarded(int x) pre(x > 0) {
  return x;
}

// ENFORCE-LABEL: define {{.*}} @_Z7guardedi(
// ENFORCE:   %__contract_violation = alloca %"class.std::contracts::contract_violation"
// ENFORCE: contract.handler:
// ENFORCE:   call void @llvm.memcpy.p0.p0.i64(ptr align 8 %__contract_violation, ptr align 8 @__const._Z7guardedi.__contract_violation, i64 32, i1 false)
// ENFORCE:   call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE(ptr {{.*}} %__contract_violation)
// ENFORCE:   call void @abort()
