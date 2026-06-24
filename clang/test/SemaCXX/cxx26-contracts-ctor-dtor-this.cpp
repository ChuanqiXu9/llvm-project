// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

namespace std::contracts {
  enum class contract_kind : unsigned char { pre, post, assert_kind };
  enum class detection_mode_t : unsigned char { predicate_false };
  class contract_violation {
    const char* _M_file; const char* _M_function; const char* _M_comment;
    unsigned int _M_line; contract_kind _M_kind; detection_mode_t _M_detection_mode;
  };
  void handle_contract_violation(const contract_violation&);
}

struct CtorThisExplicit {
  int x;
  CtorThisExplicit() pre(this->x >= 0) {} // expected-error {{constructor precondition cannot reference 'this'}}
};

struct CtorThisImplicit {
  int x;
  CtorThisImplicit() pre(x >= 0) {} // expected-error {{constructor precondition cannot reference 'this'}}
};

struct DtorThisExplicit {
  int x;
  ~DtorThisExplicit() post(this->x >= 0) {} // expected-error {{destructor postcondition cannot reference 'this'}}
};

struct DtorThisImplicit {
  int x;
  ~DtorThisImplicit() post(x >= 0) {} // expected-error {{destructor postcondition cannot reference 'this'}}
};