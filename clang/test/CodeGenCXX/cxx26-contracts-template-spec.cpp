// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -emit-llvm -o - %s | FileCheck %s

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
  void handle_contract_violation(const contract_violation&);
}

// Test 1: Explicit specialization inherits pre-condition
template<typename T> T add(T x, T y) pre(x > T{}) pre(y > T{}) { return x + y; }
template<> int add<int>(int x, int y) { return x + y; }

// CHECK-LABEL: define{{.*}} @_Z3addIiET_S0_S0_
// CHECK: icmp sgt i32
// CHECK: br i1 {{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE
// CHECK: call void @abort()
// CHECK: icmp sgt i32
// CHECK: br i1 {{.*}}, label %contract.cont{{.*}}, label %contract.handler{{.*}}
// CHECK: contract.handler{{.*}}:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE
// CHECK: call void @abort()

// Test 2: Explicit specialization inherits post-condition
template<typename T> T multiply(T x, T y) post(r: r >= T{}) { return x * y; }
template<> int multiply<int>(int x, int y) { return x * y; }

// CHECK-LABEL: define{{.*}} @_Z8multiplyIiET_S0_S0_
// CHECK: icmp sge i32
// CHECK: br i1 {{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE
// CHECK: call void @abort()

// Test 3: Explicit specialization without contracts (should inherit from primary)
template<typename T> T divide(T x, T y) pre(y != T{}) { return x / y; }
template<> int divide<int>(int x, int y) { return x / y; }

// CHECK-LABEL: define{{.*}} @_Z6divideIiET_S0_S0_
// CHECK: icmp ne i32
// CHECK: br i1 {{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK: call void @_ZNSt9contracts25handle_contract_violationERKNS_18contract_violationE
// CHECK: call void @abort()

int main() {
  add(5, 10);
  multiply(5, 10);
  divide(10, 5);
  return 0;
}
