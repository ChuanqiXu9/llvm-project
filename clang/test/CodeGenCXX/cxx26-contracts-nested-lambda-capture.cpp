// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s

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

namespace nested_contract_capture {

int explicit_outer_copy(int x)
    pre(([x] { return [=] { return x > 0; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}explicit_outer_copy
// CHECK: %[[COPY_OUTER_OBJ:.*]] = alloca %[[COPY_OUTER_TY:[^, ]+]]
// CHECK: %[[COPY_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[COPY_OUTER_TY]], ptr %[[COPY_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[COPY_VALUE:.*]] = load i32, ptr %x.addr
// CHECK: store i32 %[[COPY_VALUE]], ptr %[[COPY_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[COPY_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[COPY_OUTER_CALL]]"
// CHECK: %[[COPY_INNER_OBJ:.*]] = alloca %[[COPY_INNER_TY:[^, ]+]]
// CHECK: %[[COPY_INNER_FIELD:.*]] = getelementptr {{.*}} %[[COPY_INNER_TY]], ptr %[[COPY_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[COPY_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[COPY_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[COPY_NESTED_VALUE:.*]] = load i32, ptr %[[COPY_OUTER_FIELD_IN_OP]]
// CHECK: store i32 %[[COPY_NESTED_VALUE]], ptr %[[COPY_INNER_FIELD]]
// CHECK: call {{.*}} @"[[COPY_INNER_CALL:[^"]+]]"

int explicit_outer_ref(int x)
    pre(([&x] { return [&] { return x > 0; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}explicit_outer_ref
// CHECK: %[[REF_OUTER_OBJ:.*]] = alloca %[[REF_OUTER_TY:[^, ]+]]
// CHECK: %[[REF_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[REF_OUTER_TY]], ptr %[[REF_OUTER_OBJ]], i32 0, i32 0
// CHECK: store ptr %x.addr, ptr %[[REF_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[REF_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[REF_OUTER_CALL]]"
// CHECK: %[[REF_INNER_OBJ:.*]] = alloca %[[REF_INNER_TY:[^, ]+]]
// CHECK: %[[REF_INNER_FIELD:.*]] = getelementptr {{.*}} %[[REF_INNER_TY]], ptr %[[REF_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[REF_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[REF_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[REF_POINTER:.*]] = load ptr, ptr %[[REF_OUTER_FIELD_IN_OP]]
// CHECK: store ptr %[[REF_POINTER]], ptr %[[REF_INNER_FIELD]]
// CHECK: call {{.*}} @"[[REF_INNER_CALL:[^"]+]]"

int implicit_chain(int x)
    pre(([=] { return [=] { return x > 0; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}implicit_chain
// CHECK: %[[IMPLICIT_OUTER_OBJ:.*]] = alloca %[[IMPLICIT_OUTER_TY:[^, ]+]]
// CHECK: %[[IMPLICIT_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[IMPLICIT_OUTER_TY]], ptr %[[IMPLICIT_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[IMPLICIT_VALUE:.*]] = load i32, ptr %x.addr
// CHECK: store i32 %[[IMPLICIT_VALUE]], ptr %[[IMPLICIT_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[IMPLICIT_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[IMPLICIT_OUTER_CALL]]"
// CHECK: %[[IMPLICIT_INNER_OBJ:.*]] = alloca %[[IMPLICIT_INNER_TY:[^, ]+]]
// CHECK: %[[IMPLICIT_INNER_FIELD:.*]] = getelementptr {{.*}} %[[IMPLICIT_INNER_TY]], ptr %[[IMPLICIT_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[IMPLICIT_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[IMPLICIT_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[IMPLICIT_NESTED_VALUE:.*]] = load i32, ptr %[[IMPLICIT_OUTER_FIELD_IN_OP]]
// CHECK: store i32 %[[IMPLICIT_NESTED_VALUE]], ptr %[[IMPLICIT_INNER_FIELD]]
// CHECK: call {{.*}} @"[[IMPLICIT_INNER_CALL:[^"]+]]"

int ref_to_copy(int x)
    pre(([&x] { return [=] { return x > 0; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}ref_to_copy
// CHECK: %[[RTC_OUTER_OBJ:.*]] = alloca %[[RTC_OUTER_TY:[^, ]+]]
// CHECK: %[[RTC_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[RTC_OUTER_TY]], ptr %[[RTC_OUTER_OBJ]], i32 0, i32 0
// CHECK: store ptr %x.addr, ptr %[[RTC_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[RTC_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[RTC_OUTER_CALL]]"
// CHECK: %[[RTC_INNER_OBJ:.*]] = alloca %[[RTC_INNER_TY:[^, ]+]]
// CHECK: %[[RTC_INNER_FIELD:.*]] = getelementptr {{.*}} %[[RTC_INNER_TY]], ptr %[[RTC_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[RTC_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[RTC_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[RTC_POINTER:.*]] = load ptr, ptr %[[RTC_OUTER_FIELD_IN_OP]]
// CHECK: %[[RTC_VALUE:.*]] = load i32, ptr %[[RTC_POINTER]]
// CHECK: store i32 %[[RTC_VALUE]], ptr %[[RTC_INNER_FIELD]]
// CHECK: call {{.*}} @"[[RTC_INNER_CALL:[^"]+]]"

int copy_to_ref(int x)
    pre(([=] { return [&] { return x > 0; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}copy_to_ref
// CHECK: %[[CTR_OUTER_OBJ:.*]] = alloca %[[CTR_OUTER_TY:[^, ]+]]
// CHECK: %[[CTR_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[CTR_OUTER_TY]], ptr %[[CTR_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[CTR_VALUE:.*]] = load i32, ptr %x.addr
// CHECK: store i32 %[[CTR_VALUE]], ptr %[[CTR_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[CTR_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[CTR_OUTER_CALL]]"
// CHECK: %[[CTR_INNER_OBJ:.*]] = alloca %[[CTR_INNER_TY:[^, ]+]]
// CHECK: %[[CTR_INNER_FIELD:.*]] = getelementptr {{.*}} %[[CTR_INNER_TY]], ptr %[[CTR_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[CTR_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[CTR_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: store ptr %[[CTR_OUTER_FIELD_IN_OP]], ptr %[[CTR_INNER_FIELD]]
// CHECK: call {{.*}} @"[[CTR_INNER_CALL:[^"]+]]"

int three_levels(int x)
    pre(([=] { return [=] { return [=] { return x > 0; }(); }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}three_levels
// CHECK: %[[THREE_OUTER_OBJ:.*]] = alloca %[[THREE_OUTER_TY:[^, ]+]]
// CHECK: %[[THREE_OUTER_FIELD:.*]] = getelementptr {{.*}} %[[THREE_OUTER_TY]], ptr %[[THREE_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[THREE_VALUE:.*]] = load i32, ptr %x.addr
// CHECK: store i32 %[[THREE_VALUE]], ptr %[[THREE_OUTER_FIELD]]
// CHECK: call {{.*}} @"[[THREE_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[THREE_OUTER_CALL]]"
// CHECK: %[[THREE_MIDDLE_OBJ:.*]] = alloca %[[THREE_MIDDLE_TY:[^, ]+]]
// CHECK: %[[THREE_MIDDLE_FIELD:.*]] = getelementptr {{.*}} %[[THREE_MIDDLE_TY]], ptr %[[THREE_MIDDLE_OBJ]], i32 0, i32 0
// CHECK: %[[THREE_OUTER_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[THREE_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[THREE_MIDDLE_VALUE:.*]] = load i32, ptr %[[THREE_OUTER_FIELD_IN_OP]]
// CHECK: store i32 %[[THREE_MIDDLE_VALUE]], ptr %[[THREE_MIDDLE_FIELD]]
// CHECK: call {{.*}} @"[[THREE_MIDDLE_CALL:[^"]+]]"

long post_nested(const long x)
    post(r: ([=] { return [=] { return r >= x; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}post_nested
// CHECK: %[[POST_OUTER_OBJ:.*]] = alloca %[[POST_OUTER_TY:[^, ]+]]
// CHECK: %[[POST_R_FIELD:.*]] = getelementptr {{.*}} %[[POST_OUTER_TY]], ptr %[[POST_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[POST_R:.*]] = load i64, ptr %retval
// CHECK: store i64 %[[POST_R]], ptr %[[POST_R_FIELD]]
// CHECK: %[[POST_X_FIELD:.*]] = getelementptr {{.*}} %[[POST_OUTER_TY]], ptr %[[POST_OUTER_OBJ]], i32 0, i32 1
// CHECK: %[[POST_X:.*]] = load i64, ptr %x.addr
// CHECK: store i64 %[[POST_X]], ptr %[[POST_X_FIELD]]
// CHECK: call {{.*}} @"[[POST_OUTER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[POST_OUTER_CALL]]"
// CHECK: %[[POST_INNER_OBJ:.*]] = alloca %[[POST_INNER_TY:[^, ]+]]
// CHECK: %[[POST_INNER_R_FIELD:.*]] = getelementptr {{.*}} %[[POST_INNER_TY]], ptr %[[POST_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[POST_OUTER_R_FIELD:.*]] = getelementptr {{.*}} %[[POST_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[POST_INNER_R:.*]] = load i64, ptr %[[POST_OUTER_R_FIELD]]
// CHECK: store i64 %[[POST_INNER_R]], ptr %[[POST_INNER_R_FIELD]]
// CHECK: %[[POST_INNER_X_FIELD:.*]] = getelementptr {{.*}} %[[POST_INNER_TY]], ptr %[[POST_INNER_OBJ]], i32 0, i32 1
// CHECK: %[[POST_OUTER_X_FIELD:.*]] = getelementptr {{.*}} %[[POST_OUTER_TY]], ptr %{{.*}}, i32 0, i32 1
// CHECK: %[[POST_INNER_X:.*]] = load i64, ptr %[[POST_OUTER_X_FIELD]]
// CHECK: store i64 %[[POST_INNER_X]], ptr %[[POST_INNER_X_FIELD]]
// CHECK: call {{.*}} @"[[POST_INNER_CALL:[^"]+]]"

struct PointerResult {};

PointerResult *post_pointer_nested(PointerResult *const x)
    post(r: ([=] { return [=] { return r == x; }(); })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}post_pointer_nested
// CHECK: store ptr %{{.*}}, ptr %{{.*}}
// CHECK: call {{.*}}

auto post_deduced_pointer_capture(PointerResult *const x)
    post(r: ([=] { return r == x; })()) {
  return x;
}

// CHECK-LABEL: define {{.*}}post_deduced_pointer_capture
// CHECK: store ptr %{{.*}}, ptr %{{.*}}
// CHECK: call {{.*}}

struct LateParsedPostcondition {
  long nested(const long x)
      post(r: ([=] { return [=] { return r >= x; }(); })()) {
    return x;
  }

  PointerResult *pointer_nested(PointerResult *const x)
      post(r: ([=] { return [=] { return r == x; }(); })()) {
    return x;
  }
};

long call_late_parsed_postcondition(long x) {
  return LateParsedPostcondition{}.nested(x);
}

PointerResult *call_late_parsed_pointer_postcondition(PointerResult *x) {
  return LateParsedPostcondition{}.pointer_nested(x);
}

// CHECK-LABEL: define {{.*}}LateParsedPostcondition6nested
// CHECK: %[[LATE_OUTER_OBJ:.*]] = alloca %[[LATE_OUTER_TY:[^, ]+]]
// CHECK: %[[LATE_R_FIELD:.*]] = getelementptr {{.*}} %[[LATE_OUTER_TY]], ptr %[[LATE_OUTER_OBJ]], i32 0, i32 0
// CHECK: %[[LATE_R:.*]] = load i64, ptr %retval
// CHECK: store i64 %[[LATE_R]], ptr %[[LATE_R_FIELD]]
// CHECK: %[[LATE_X_FIELD:.*]] = getelementptr {{.*}} %[[LATE_OUTER_TY]], ptr %[[LATE_OUTER_OBJ]], i32 0, i32 1
// CHECK: %[[LATE_X:.*]] = load i64, ptr %x.addr
// CHECK: store i64 %[[LATE_X]], ptr %[[LATE_X_FIELD]]
// CHECK: call {{.*}} @[[LATE_OUTER_CALL:[^ (]+]]

} // namespace nested_contract_capture

// The nested call operators are emitted after the enclosing functions.

// CHECK: define internal {{.*}} @"[[COPY_INNER_CALL]]"
// CHECK: %[[COPY_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[COPY_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: load i32, ptr %[[COPY_FINAL_FIELD]]

// CHECK: define internal {{.*}} @"[[REF_INNER_CALL]]"
// CHECK: %[[REF_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[REF_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[REF_FINAL_POINTER:.*]] = load ptr, ptr %[[REF_FINAL_FIELD]]
// CHECK: load i32, ptr %[[REF_FINAL_POINTER]]

// CHECK: define internal {{.*}} @"[[IMPLICIT_INNER_CALL]]"
// CHECK: %[[IMPLICIT_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[IMPLICIT_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: load i32, ptr %[[IMPLICIT_FINAL_FIELD]]

// CHECK: define internal {{.*}} @"[[RTC_INNER_CALL]]"
// CHECK: %[[RTC_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[RTC_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: load i32, ptr %[[RTC_FINAL_FIELD]]

// CHECK: define internal {{.*}} @"[[CTR_INNER_CALL]]"
// CHECK: %[[CTR_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[CTR_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[CTR_FINAL_POINTER:.*]] = load ptr, ptr %[[CTR_FINAL_FIELD]]
// CHECK: load i32, ptr %[[CTR_FINAL_POINTER]]

// CHECK: define internal {{.*}} @"[[THREE_MIDDLE_CALL]]"
// CHECK: %[[THREE_INNER_OBJ:.*]] = alloca %[[THREE_INNER_TY:[^, ]+]]
// CHECK: %[[THREE_INNER_FIELD:.*]] = getelementptr {{.*}} %[[THREE_INNER_TY]], ptr %[[THREE_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[THREE_MIDDLE_FIELD_IN_OP:.*]] = getelementptr {{.*}} %[[THREE_MIDDLE_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[THREE_INNER_VALUE:.*]] = load i32, ptr %[[THREE_MIDDLE_FIELD_IN_OP]]
// CHECK: store i32 %[[THREE_INNER_VALUE]], ptr %[[THREE_INNER_FIELD]]
// CHECK: call {{.*}} @"[[THREE_INNER_CALL:[^"]+]]"
// CHECK: define internal {{.*}} @"[[THREE_INNER_CALL]]"
// CHECK: %[[THREE_FINAL_FIELD:.*]] = getelementptr {{.*}} %[[THREE_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: load i32, ptr %[[THREE_FINAL_FIELD]]

// CHECK: define internal {{.*}} @"[[POST_INNER_CALL]]"
// CHECK: %[[POST_FINAL_R_FIELD:.*]] = getelementptr {{.*}} %[[POST_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[POST_FINAL_R:.*]] = load i64, ptr %[[POST_FINAL_R_FIELD]]
// CHECK: %[[POST_FINAL_X_FIELD:.*]] = getelementptr {{.*}} %[[POST_INNER_TY]], ptr %{{.*}}, i32 0, i32 1
// CHECK: %[[POST_FINAL_X:.*]] = load i64, ptr %[[POST_FINAL_X_FIELD]]
// CHECK: icmp sge i64 %[[POST_FINAL_R]], %[[POST_FINAL_X]]

// CHECK: define {{.*}} @[[LATE_OUTER_CALL]]
// CHECK: %[[LATE_INNER_OBJ:.*]] = alloca %[[LATE_INNER_TY:[^, ]+]]
// CHECK: %[[LATE_INNER_R_FIELD:.*]] = getelementptr {{.*}} %[[LATE_INNER_TY]], ptr %[[LATE_INNER_OBJ]], i32 0, i32 0
// CHECK: %[[LATE_OUTER_R_FIELD:.*]] = getelementptr {{.*}} %[[LATE_OUTER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[LATE_INNER_R:.*]] = load i64, ptr %[[LATE_OUTER_R_FIELD]]
// CHECK: store i64 %[[LATE_INNER_R]], ptr %[[LATE_INNER_R_FIELD]]
// CHECK: %[[LATE_INNER_X_FIELD:.*]] = getelementptr {{.*}} %[[LATE_INNER_TY]], ptr %[[LATE_INNER_OBJ]], i32 0, i32 1
// CHECK: %[[LATE_OUTER_X_FIELD:.*]] = getelementptr {{.*}} %[[LATE_OUTER_TY]], ptr %{{.*}}, i32 0, i32 1
// CHECK: %[[LATE_INNER_X:.*]] = load i64, ptr %[[LATE_OUTER_X_FIELD]]
// CHECK: store i64 %[[LATE_INNER_X]], ptr %[[LATE_INNER_X_FIELD]]
// CHECK: call {{.*}} @[[LATE_INNER_CALL:[^ (]+]]
// CHECK: define {{.*}} @[[LATE_INNER_CALL]]
// CHECK: %[[LATE_FINAL_R_FIELD:.*]] = getelementptr {{.*}} %[[LATE_INNER_TY]], ptr %{{.*}}, i32 0, i32 0
// CHECK: %[[LATE_FINAL_R:.*]] = load i64, ptr %[[LATE_FINAL_R_FIELD]]
// CHECK: %[[LATE_FINAL_X_FIELD:.*]] = getelementptr {{.*}} %[[LATE_INNER_TY]], ptr %{{.*}}, i32 0, i32 1
// CHECK: %[[LATE_FINAL_X:.*]] = load i64, ptr %[[LATE_FINAL_X_FIELD]]
// CHECK: icmp sge i64 %[[LATE_FINAL_R]], %[[LATE_FINAL_X]]
