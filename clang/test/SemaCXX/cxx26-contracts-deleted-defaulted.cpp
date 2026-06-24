// RUN: %clang_cc1 -std=c++2c -fcontracts -verify %s

namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  class contract_violation {
    const char* _M_file;
    const char* _M_function;
    const char* _M_comment;
    unsigned int _M_line;
    contract_kind _M_kind;
    detection_mode_t _M_detection_mode;
  };
  void handle_contract_violation(const contract_violation&);
}

// Test 1: Deleted function with pre-condition
void deleted_pre() pre(true) = delete; // expected-error {{contracts may not be specified on a deleted function}}

// Test 2: Deleted function with post-condition
void deleted_post() post(true) = delete; // expected-error {{contracts may not be specified on a deleted function}}

// Test 3: Defaulted copy constructor with pre-condition
struct DefaultedCopy {
  DefaultedCopy() = default;
  DefaultedCopy(const DefaultedCopy&) pre(true) = default; // expected-error {{contracts may not be specified on a defaulted function}}
};

// Test 4: Defaulted move constructor with post-condition
struct DefaultedMove {
  DefaultedMove() = default;
  DefaultedMove(DefaultedMove&&) post(true) = default; // expected-error {{contracts may not be specified on a defaulted function}}
};

// Test 5: Defaulted assignment operator with pre-condition
struct DefaultedAssign {
  DefaultedAssign& operator=(const DefaultedAssign&) pre(true) = default; // expected-error {{contracts may not be specified on a defaulted function}}
};

// Test 6: Defaulted destructor with pre-condition
struct DefaultedDestructor {
  ~DefaultedDestructor() pre(true) = default; // expected-error {{contracts may not be specified on a defaulted function}}
};

// Test 7: Non-deleted, non-defaulted function with contract (should be fine)
void normal_with_contract(int x) pre(x > 0) {} // no error

// Test 8: Deleted function without contract (should be fine)
void deleted_no_contract() = delete; // no error

// Test 9: Defaulted function without contract (should be fine)
struct DefaultedNoContract {
  DefaultedNoContract() = default; // no error
  ~DefaultedNoContract() = default; // no error
};
