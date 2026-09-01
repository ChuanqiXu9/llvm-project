// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   -fcxx-exceptions -fexceptions -triple x86_64-linux-gnu \
// RUN:   -emit-llvm -o - %s | FileCheck %s

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

struct PredicateTemporary {
  ~PredicateTemporary();
  bool valid() const;
};

struct Result {
  Result();
  ~Result();
  PredicateTemporary predicate() const;
};

Result ordinary() post(result : result.predicate().valid()) {
  return Result();
}

struct DelayedMember {
  Result get() post(result : result.predicate().valid()) {
    return Result();
  }
};

template <class T>
T templated() post(result : result.predicate().valid()) {
  return T();
}

void asserted() {
  contract_assert(PredicateTemporary{}.valid());
}

void instantiate(DelayedMember &member) {
  ordinary();
  member.get();
  templated<Result>();
  asserted();
}

// Each potentially-throwing call that determines a predicate has an unwind
// edge which destroys its temporary. The temporary is also destroyed on the
// normal path before control branches to the contract handler.

// CHECK-LABEL: define{{.*}} @_Z8ordinaryv(
// CHECK: invoke{{.*}} @_ZNK18PredicateTemporary5validEv
// CHECK: call void @_ZN18PredicateTemporaryD1Ev
// CHECK-NEXT: br i1
// CHECK: landingpad
// CHECK: call void @_ZN18PredicateTemporaryD1Ev

// CHECK-LABEL: define{{.*}} @_Z8assertedv(
// CHECK: invoke{{.*}} @_ZNK18PredicateTemporary5validEv
// CHECK: call void @_ZN18PredicateTemporaryD1Ev
// CHECK-NEXT: br i1
// CHECK: landingpad
// CHECK: call void @_ZN18PredicateTemporaryD1Ev

// CHECK-LABEL: define linkonce_odr{{.*}} @_ZN13DelayedMember3getEv(
// CHECK: invoke{{.*}} @_ZNK18PredicateTemporary5validEv
// CHECK: call void @_ZN18PredicateTemporaryD1Ev
// CHECK-NEXT: br i1
// CHECK: landingpad
// CHECK: call void @_ZN18PredicateTemporaryD1Ev

// CHECK-LABEL: define linkonce_odr{{.*}} @_Z9templatedI6ResultET_v(
// CHECK: invoke{{.*}} @_ZNK18PredicateTemporary5validEv
// CHECK: call void @_ZN18PredicateTemporaryD1Ev
// CHECK-NEXT: br i1
// CHECK: landingpad
// CHECK: call void @_ZN18PredicateTemporaryD1Ev
