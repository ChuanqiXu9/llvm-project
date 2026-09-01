// RUN: %clang_cc1 -std=c++2c -fcontracts -fblocks -Wno-objc-root-class \
// RUN:   -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2c -fcontracts -fblocks -Wno-objc-root-class \
// RUN:   -emit-llvm -o /dev/null %s
// expected-no-diagnostics

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

@interface ContractObject
- (void)check;
@end

@implementation ContractObject
- (void)check {
  contract_assert(true);
}
@end

void check_block() {
  void (^block)() = ^{ contract_assert(true); };
  block();
}
