// RUN: %clang_cc1 -std=c++2c -fcontracts -verify %s

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
}
void handle_contract_violation(const std::contracts::contract_violation&);

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

struct DefaultedDefaultCtor { DefaultedDefaultCtor() pre(true) = default; }; // expected-error {{contracts may not be specified on a defaulted function}}
struct DefaultedCompare { int value; bool operator==(const DefaultedCompare&) const pre(true) = default; }; // expected-error {{contracts may not be specified on a defaulted function}}
struct DeletedCallOperator { void operator()() pre(true) = delete; }; // expected-error {{contracts may not be specified on a deleted function}}
struct DeletedSubscriptOperator { int operator[](int) pre(true) = delete; }; // expected-error {{contracts may not be specified on a deleted function}}
struct DeletedUnaryOperator { bool operator!() const post(true) = delete; }; // expected-error {{contracts may not be specified on a deleted function}}
struct DefaultedMoveAssign { DefaultedMoveAssign &operator=(DefaultedMoveAssign&&) post(true) = default; }; // expected-error {{contracts may not be specified on a defaulted function}}
