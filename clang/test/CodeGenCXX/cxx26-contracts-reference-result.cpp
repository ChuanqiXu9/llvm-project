// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -emit-llvm -o - %s | FileCheck %s

namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
}

namespace std::contracts {
enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
enum class evaluation_semantic : unsigned short { ignore = 1, observe = 2, enforce = 3, quick_enforce = 4 };
enum class detection_mode : unsigned short { predicate_false = 1, evaluation_exception = 2 };
class contract_violation {
  unsigned short _M_version;
  assertion_kind _M_assertion_kind;
  evaluation_semantic _M_evaluation_semantic;
  detection_mode _M_detection_mode;
  const char *_M_comment;
  const void *_M_src_loc_ptr;
  void *_M_ext;
};
}

void handle_contract_violation(
    const std::contracts::contract_violation &) {}

struct Value {
  int storage;

  int size() const { return storage; }
};

const Value &get_value(const Value &value)
  post(result: result.size() == 1) {
  return value;
}

// CHECK-LABEL: define{{.*}} ptr @_Z9get_valueRK5Value
// CHECK: %[[RETURNED:.*]] = load ptr, ptr %retval
// CHECK: call{{.*}} i32 @_ZNK5Value4sizeEv(ptr {{.*}} %[[RETURNED]])

int main() {
  Value value{1};
  return get_value(value).size() != 1;
}
