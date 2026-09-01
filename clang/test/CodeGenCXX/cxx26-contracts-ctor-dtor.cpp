// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()
// CHECK: contract.handler{{.*}}:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()

// CHECK-LABEL: define {{.*}} @_ZN1SD2Ev(
// CHECK: contract.handler:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
// CHECK: call void @abort()
// CHECK: contract.handler{{.*}}:
// CHECK: call void @_Z25handle_contract_violationRKNSt9contracts18contract_violationE(
