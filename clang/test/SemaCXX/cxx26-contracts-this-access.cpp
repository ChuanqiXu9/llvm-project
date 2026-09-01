// RUN: %clang_cc1 -std=c++2c -fcontracts -fsyntax-only -verify %s

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

// P2900R14 §3.4.1: Contract predicates can access 'this' and members
// Delayed parsing is now implemented, so these should all work.

struct S {
  int x;
  int y;

  // Member function preconditions - should all work now
  void f1() pre(this->x > 0);
  void f2() pre(x > 0);
  void f3() pre(this->x > 0 && this->y > 0);

  // Member function postconditions - should all work now
  int g1() post(this->x > 0);
  int g2() post(r: r > this->x);

  // Const member functions - should work now
  void h1() const pre(this->x > 0);
  void h2() const pre(x > 0);
  int h3() const post(r: r >= this->x);



  // Static member functions - no 'this' available, so normal error
  static void s1() pre(x > 0);  // expected-error {{invalid use of member 'x' in static member function}}
};

struct StaticCompleteResult {
  int size() const { return 1; }

  static StaticCompleteResult make()
    post(result: result.size() == 1) {
    return {};
  }

  template <class T>
  static StaticCompleteResult make_template(T)
    post(result: result.size() == 1) {
    return {};
  }
};

struct QualifiedStaticContract {
  int value;

  template <class T>
  static int specialized();
};

template <>
int QualifiedStaticContract::specialized<int>()
  pre(this->value >= 0) { // expected-error {{invalid use of 'this' outside of a non-static member function}}
  return 0;
}

template <class T>
struct LateMemberTemplateContract {
  LateMemberTemplateContract()
    post(this->size() == 0 && sizeof(*this) >= sizeof(T)) {}

  int size() const { return 0; }
};

LateMemberTemplateContract<int> late_member_template_contract;

template <class T>
struct LazyMemberContract {
  void unused() pre(sizeof(typename T::missing) != 0) {}
};

// Instantiating the class does not instantiate the contract of an unused
// non-deleted member function.
LazyMemberContract<int> lazy_member_contract;

struct U {
  int x;
  ~U() pre(x >= 0);
};

struct RedeclOK {
  void f(int x) pre(x > 0);
};

void RedeclOK::f(int x) pre(x > 0) {}

struct RedeclMismatch {
  void f(int x) pre(x > 0); // expected-note {{previous declaration is here}}
};

void RedeclMismatch::f(int x) pre(x > 1) {} // expected-error {{contracts on function redeclaration do not match the previous declaration}}

struct RedeclLambdaOK {
  void f() pre([] { return true; }());
};

// Distinct lambda-expressions have distinct closure types, so this program is
// ill-formed; the standard does not require a diagnostic for this case. Accept
// equivalent lambda syntax so module implementation units remain usable.
void RedeclLambdaOK::f() pre([] { return true; }()) {}

struct RedeclLambdaMismatch {
  void f() pre([] { return true; }()); // expected-note {{previous declaration is here}}
};

void RedeclLambdaMismatch::f() pre([] { return false; }()) {} // expected-error {{contracts on function redeclaration do not match the previous declaration}}

struct EmptyPredicate {
  void f() pre(); // expected-error {{expected expression}}
  void g() pre(this->x > 0);
  int x;
};

struct InvalidDelayedContract {
  void f(int x) pre(++x > 0) pre(x > 0); // expected-error {{read-only variable is not assignable}} expected-note {{previous declaration is here}}
};

void InvalidDelayedContract::f(int x) pre(x > 1) {} // expected-error {{contracts on function redeclaration do not match the previous declaration}}



void instantiate_member_contracts() {
  S s{1, 2};
}
