// RUN: %clang_analyze_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   -analyzer-checker=core,debug.ExprInspection -verify=enforce %s
// RUN: %clang_analyze_cc1 -std=c++2c -fcontracts -fcontract-mode=ignore \
// RUN:   -analyzer-checker=core,debug.ExprInspection -verify=ignore %s

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
void clang_analyzer_eval(bool);

void enforce_contract_assert(int x) {
  contract_assert(x > 0);
  // enforce-warning@+2{{TRUE}}
  // ignore-warning@+1{{TRUE}} ignore-warning@+1{{FALSE}}
  clang_analyzer_eval(x > 0);
}

void ignore_contract_assert_side_effect(int *p) {
  contract_assert((*p = 1));
  // enforce-warning@+2{{TRUE}}
  // ignore-warning@+1{{TRUE}} ignore-warning@+1{{FALSE}}
  clang_analyzer_eval(*p == 1);
}
