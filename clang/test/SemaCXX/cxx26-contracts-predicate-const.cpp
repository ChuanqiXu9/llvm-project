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

// An id-expression naming an object declared outside the contract assertion
// has type const T. This is a type rule, not a blanket ban on side effects.
int global_var = 0;

void read_only(int x) pre(x > 0);
void assign(int x) pre((x = 0) == 0); // expected-error {{read-only variable is not assignable}}
void increment(int x) pre(++x > 0); // expected-error {{read-only variable is not assignable}}
void global_increment() pre(++global_var > 0); // expected-error {{read-only variable is not assignable}}

// The pointer object is const, not its pointee.
void mutate_pointee(int *p) pre((*p = 5) > 0) pre(++*p > 0);

struct S {
  int value;
  int getValue() const { return value; }
  bool setValueAndCheck(int v) { value = v; return value > 0; } // expected-note {{'setValueAndCheck' declared here}}
};

void const_call(S &s) pre(s.getValue() > 0);
void nonconst_call(S &s) pre(s.setValueAndCheck(5)); // expected-error {{'this' argument to member function 'setValueAndCheck' has type 'const S'}}

// The implicit const type participates in overload resolution.
struct Overload {
  bool select() = delete;
  bool select() const { return true; }
};
void overload(Overload value) pre(value.select());

// Variables declared within the predicate and mutable copy captures remain
// mutable. A reference capture still denotes the outside entity.
void lambda_copy(int n)
  pre([=]() mutable { ++n; int local = 0; ++local; return n > local; }());
void lambda_ref(int n)
  pre([&n]() mutable { ++n; return true; }()); // expected-error {{cannot assign}}

void lambda_global()
  pre([=]() mutable { ++global_var; return true; }()); // expected-error {{read-only variable is not assignable}}

struct CopyFromNonConst {
  CopyFromNonConst(CopyFromNonConst &);
};
void lambda_copy_initialization(CopyFromNonConst value)
  pre([value]() mutable { return true; }());

struct MemberObject {
  int value;
  bool modify() { return ++value > 0; } // expected-note 2 {{'modify' declared here}}
  void direct() pre(++value > 0); // expected-error {{read-only variable is not assignable}}
  void direct_this() pre(this->modify()); // expected-error {{'this' argument to member function 'modify' has type 'const MemberObject'}}
  void captured_this()
    pre([this]() mutable { return this->modify(); }()); // expected-error {{'this' argument to member function 'modify' has type 'const MemberObject'}}
  void copied_this()
    pre([*this]() mutable { ++value; return true; }());
};

// Explicit casts are governed by the ordinary language rules; the contract
// implementation must not reject the enclosing assignment after the fact.
int explicit_const_cast(const int x)
  post(r: (const_cast<int &>(x) = r) == r);

// The implicit-const context applies to the predicate expression itself, not
// to declarations introduced while satisfying a nested template constraint.
struct ConstraintValue {};

template <typename T>
concept SelfAssignable = requires(T lhs, T rhs) {
  lhs = rhs;
};

template <SelfAssignable T>
bool accepts_self_assignable(T) { return true; }

void constraint_instantiation(ConstraintValue value)
  pre(accepts_self_assignable(value));

// Instantiating a function template from a predicate must not make that
// function's parameters or locals implicitly const.
template <typename T>
bool mutate_parameter(T value) {
  ++value;
  return true;
}

void function_template_instantiation(int value)
  pre(mutate_parameter(value));

template <typename T>
struct MutableIterator {
  T value;

  constexpr MutableIterator &operator++() {
    ++value;
    return *this;
  }
};

template <typename T>
constexpr bool advance_copy(T value) {
  ++value;
  return true;
}

void member_function_template_instantiation(int value)
  pre(advance_copy(MutableIterator<int>{value}));

// The implicit const qualification of this must participate in overload
// resolution, including when a class template member's predicate is
// instantiated after the complete class is known.
template <typename T>
struct ThisOverload {
  void erase() pre(begin() != nullptr) {}

  const T *begin() const { return nullptr; }
  T *begin() { return nullptr; }
};

void instantiate_this_overload() {
  ThisOverload<int> value;
  value.erase();
}

// A specialization can be requested while its primary class template is
// still being defined. Delayed contract parsing must nevertheless use the
// complete overload set and the predicate's const-qualified this type.
template <typename T>
struct EarlyThisOverload;

using EarlyThisOverloadInt = EarlyThisOverload<int>;

template <typename T>
struct EarlyThisOverload {
  static EarlyThisOverloadInt force(EarlyThisOverloadInt value) {
    return value;
  }

  using iterator = T *;
  iterator erase(iterator first, iterator last)
    pre(first >= begin()) pre(last <= end()) {
    return first;
  }

  const T *begin() const { return nullptr; }
  const T *end() const { return nullptr; }
  T *begin() { return nullptr; }
  T *end() { return nullptr; }
};
