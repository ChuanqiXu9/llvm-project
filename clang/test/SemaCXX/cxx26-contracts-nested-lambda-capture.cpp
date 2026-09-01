// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify -verify-ignore-unexpected=note %s

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
    pre(([x] { return [=] { return x > 0; }(); })());

int explicit_outer_ref(int x)
    pre(([&x] { return [&] { return x > 0; }(); })());

int implicit_chain(int x)
    pre(([=] { return [=] { return x > 0; }(); })());

int ref_to_copy(int x)
    pre(([&x] { return [=] { return x > 0; }(); })());

int copy_to_ref(int x)
    pre(([=] { return [&] { return x > 0; }(); })());

int three_levels(int x)
    pre(([=] { return [=] { return [=] { return x > 0; }(); }(); })());

long post_nested(const long x)
    post(r: ([=] { return [=] { return r >= x; }(); })());

long post_explicit_result(const long x)
    post(r: ([r] { return r > 0; })());

struct PointerResult {};

PointerResult *post_pointer_nested(PointerResult *const x)
    post(r: ([=] { return [=] { return r == x; }(); })());

auto post_deduced_pointer_capture(PointerResult *const x)
    post(r: ([=] { return r == x; })()) {
  return x;
}

struct LateParsedPostcondition {
  long nested(const long x)
      post(r: ([=] { return [=] { return r >= x; }(); })());

  PointerResult *pointer_nested(PointerResult *const x)
      post(r: ([=] { return [=] { return r == x; }(); })());
};

int missing_outer_capture(int x)
    pre(([] { return [=] { return x > 0; }(); })()); // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}}

int missing_inner_capture(int x)
    pre(([=] { return [] { return x > 0; }(); })()); // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}}

} // namespace nested_contract_capture
