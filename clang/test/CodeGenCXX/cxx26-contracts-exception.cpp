// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++2c -fcontracts -fcontract-mode=observe -fcxx-exceptions -fexceptions -emit-llvm -o - %s | FileCheck %s

namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
} // namespace std

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
  const char *_M_comment;
  const void *_M_src_loc_ptr;
  void *_M_ext;
};
} // namespace std::contracts

bool throwing_predicate();

void checked() pre(throwing_predicate()) {}

// CHECK-LABEL: define{{.*}} @_Z7checkedv
// CHECK: invoke noundef zeroext i1 @_Z18throwing_predicatev()
// CHECK-NEXT: to label %invoke.cont unwind label %lpad
// CHECK: contract.handler:
// CHECK: %[[FALSE_MODE:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 3
// CHECK-NEXT: store i16 1, ptr %[[FALSE_MODE]]
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE
// CHECK: lpad:
// CHECK: landingpad
// CHECK: catch ptr null
// CHECK: br label %contract.exception
// CHECK: contract.exception:
// CHECK: call ptr @__cxa_begin_catch
// CHECK: %[[EXCEPTION_MODE:.*]] = getelementptr {{.*}}, ptr %{{.*}}, i32 0, i32 3
// CHECK-NEXT: store i16 2, ptr %[[EXCEPTION_MODE]]
// CHECK: invoke void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE
// CHECK: call void @__cxa_end_catch
// CHECK: br label %contract.cont

// A visible replacement declaration is used directly, including its
// exception specification.
void handle_contract_violation(
    const std::contracts::contract_violation &) noexcept;

void checked_noexcept() pre(throwing_predicate()) {}

// CHECK-LABEL: define{{.*}} @_Z16checked_noexceptv
// CHECK: contract.exception:
// CHECK: call ptr @__cxa_begin_catch
// CHECK-NOT: invoke void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE
// CHECK: call void @__cxa_end_catch
