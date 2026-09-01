// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++2c -fcontracts -fcontract-mode=enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=ITANIUM
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -std=c++2c -fcontracts -fcontract-mode=enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=MS

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

void handle_contract_violation(
    const std::contracts::contract_violation &);

struct Parameter {
  int value;
  ~Parameter();
};

int read(const Parameter parameter)
    post(result: result == parameter.value) {
  return parameter.value;
}

// ITANIUM-LABEL: define{{.*}} @_Z4read9Parameter
// ITANIUM: contract.cont:
// ITANIUM-NOT: call void @_ZN9ParameterD
// ITANIUM: ret i32

// MS-LABEL: define{{.*}} @"?read@@YAHUParameter@@@Z"
// MS: contract.cont:
// MS: call {{.*}} @"??1Parameter@@QEAA@XZ"
// MS: ret i32
