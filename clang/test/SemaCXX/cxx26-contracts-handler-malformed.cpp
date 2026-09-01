// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

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
class contract_violation;
} // namespace std::contracts

// An unrelated user declaration must not affect the implementation-generated
// call to the replaceable handler symbol.
int handle_contract_violation(const std::contracts::contract_violation &);

int f(int x) pre(x > 0);

void invalid_builtin_argument(int *p) {
  // expected-error@+1 {{argument to '__builtin_contract_violation_handler' must be a pointer to 'const std::contracts::contract_violation'}}
  __builtin_contract_violation_handler(p);
}
