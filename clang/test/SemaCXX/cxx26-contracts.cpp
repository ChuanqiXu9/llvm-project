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

// === 1. Predicate bool conversion ===

struct NoBool {};
struct HasBool { explicit operator bool() const; };

int f1(int x) pre(x > 0); // ok: comparison returns bool

int f2(int x) pre(x);     // ok: int contextually converts to bool

void f3(int x) {
  contract_assert(x);      // ok
}

int f4(NoBool nb) pre(nb); // expected-error {{value of type 'const NoBool' is not contextually convertible to 'bool'}}

int f5(HasBool hb) pre(hb); // ok: explicit operator bool allowed

void f6(NoBool nb) {
  contract_assert(nb); // expected-error {{value of type 'const NoBool' is not contextually convertible to 'bool'}}
}

// === 2. Redeclaration consistency ===

int good_redecl(int x) pre(x > 0);
int good_redecl(int x); // ok: omits contracts (inherits)

int mismatch(int x) pre(x > 0);       // expected-note {{previous declaration is here}}
int mismatch(int x) pre(x > 1);       // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int no_contract_first(int x);              // expected-note {{previous declaration is here}}
int no_contract_first(int x) pre(x > 0);  // expected-error {{contracts may not be added on a function redeclaration}}

int same_contracts(const int x) pre(x > 0) post(r: r >= 0); // expected-note {{previous declaration is here}}
int same_contracts(const int x) pre(x > 0) post(r: r >= 0); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int renamed_parameter(const int x) pre(x > 0);
int renamed_parameter(const int y) pre(y > 0); // ok: corresponding parameter

int renamed_result() post(first: first > 0); // expected-note {{previous declaration is here}}
int renamed_result() post(second: second > 0); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int result_introducer_mismatch() post(result: true); // expected-note {{previous declaration is here}}
int result_introducer_mismatch() post(true); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int different_count(int x) pre(x > 0);     // expected-note {{previous declaration is here}}
int different_count(int x) pre(x > 0) pre(x < 100); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int post_mismatch(const int x) post(r: r > 0); // expected-note {{previous declaration is here}}
int post_mismatch(const int x) post(r: r > 1); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

int reordered_contracts(const int x) pre(x > 0) post(r: r >= x); // expected-note {{previous declaration is here}}
int reordered_contracts(const int x) post(r: r >= x) pre(x > 0); // expected-error {{contracts on function redeclaration do not match the previous declaration}}

// === 3. Virtual functions not supported ===

struct Base {
  virtual int compute(int x) pre(x > 0); // expected-error {{contracts may not be specified on a virtual function at this time}}
  virtual void action(); // ok: no contracts
};

struct Derived : Base {
  // An override is virtual and therefore rejected by the same rule.
  int compute(int x) override pre(x > 0); // expected-error {{contracts may not be specified on a virtual function at this time}}
  void action() override; // ok: no contracts on override
};

struct MultipleVirtualContracts {
  virtual int f(int x) pre(x > 0) post(r: r >= x) pre(x < 100); // expected-error {{contracts may not be specified on a virtual function at this time}}
};

struct DelayedVoidResult {
  DelayedVoidResult() post(result: true); // expected-error {{post-condition result name on function returning void}}
};

// === 4. Template instantiation ===

template <typename T>
T clamp(T x, const T lo, const T hi) pre(lo <= hi) post(r: r >= lo) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

void use_clamp() {
  clamp(5, 0, 10);
  clamp(3.0, 1.0, 9.0);
}

template <typename T>
T must_positive(T x) pre(x > T{}) {
  return x;
}

void use_must_positive() {
  must_positive(42);
  must_positive(3.14);
}

struct ContractOps { int v; ContractOps(const int v) pre(v >= 0) : v(v) {}
  ContractOps operator+(const ContractOps &rhs) const pre(rhs.v >= 0) post(r: r.v >= v) { return ContractOps(v + rhs.v); }
  int operator[](const int i) const pre(i >= 0) post(r: r >= v) { return v + i; }
  explicit operator bool() const pre(v >= 0) post(r: r == (v != 0)) { return v != 0; }
};
bool operator==(const ContractOps &a, const ContractOps &b) pre(a.v >= 0) pre(b.v >= 0) post(r: r == true || r == false) { return a.v == b.v; }
void use_contract_ops() { ContractOps a(1), b(2); ContractOps c = a + b; bool e = a == b; bool t = static_cast<bool>(c); int i = c[0]; (void)e; (void)t; (void)i; }

int inherited_contract_definition(int x) pre(x > 0);
int inherited_contract_definition(int x) { return x; }
struct PointerResult { int value; };
PointerResult *pointer_result()
    post(result: result != nullptr && result->value >= 0);
const PointerResult *const_pointer_result()
    post(result: result != nullptr && result->value >= 0);
PointerResult **pointer_pointer_result()
    post(result: result != nullptr && *result != nullptr);
PointerResult &reference_result() post(result: result.value >= 0);
PointerResult *&reference_to_pointer_result()
    post(result: result != nullptr && result->value >= 0);
int PointerResult::*member_pointer_result() post(result: result != nullptr);

int function_pointer_target();
int (*function_pointer_result())() post(result: result() == 0);

bool accepts_address_space_pointer(
    int *__attribute__((address_space(1))) const &);
bool accepts_address_space_pointer(int *const &) = delete;
int *__attribute__((address_space(1))) attributed_pointer_result()
    post(result: accepts_address_space_pointer(result));

void &invalid_reference_result() // expected-error {{cannot form a reference to 'void'}}
    post(result: true);

template <int Limit> int bounded_value(int x) pre(Limit > 0) pre(x >= 0) post(r: r >= 0) { return x < Limit ? x : Limit; }
void use_bounded_value() { bounded_value<1>(0); bounded_value<10>(5); }

template <typename T>
T instantiated_void_named_result() post(result: true);
// expected-error@-1 {{post-condition result name on function returning void}}
// expected-note@-2 {{candidate template ignored: substitution failure}}
void use_instantiated_void_named_result() {
  instantiated_void_named_result<void>();
  // expected-error@-1 {{no matching function for call to 'instantiated_void_named_result'}}
  // expected-note@-2 {{in instantiation of function template specialization 'instantiated_void_named_result<void>' requested here}}
}

void contract_assert_expression_forms(int x, bool flag) { contract_assert(flag ? x >= 0 : x > 0); contract_assert(((void)(x >= 0), true)); contract_assert(static_cast<bool>(x + 1)); }

struct ExplicitFalse { explicit operator bool() const; };
struct DeletedBool { explicit operator bool() const = delete; }; // expected-note 2{{'operator bool' has been explicitly marked deleted here}}
int predicate_explicit_bool(ExplicitFalse x) pre(x); // ok: explicit operator bool allowed
int predicate_deleted_bool(DeletedBool x) pre(x); // expected-error {{attempt to use a deleted function}}
void contract_assert_deleted_bool(DeletedBool x) { contract_assert(x); } // expected-error {{attempt to use a deleted function}}

int definition_repeats_same_contract(const int x) pre(x > 0);
int definition_repeats_same_contract(const int x) pre(x > 0) { return x; }

int definition_omits_post_contract(const int x) post(r: r >= x);
int definition_omits_post_contract(const int x) { return x; }

struct FriendContracts {
  int value;

  friend int friend_defined(FriendContracts obj)
      pre(obj.value >= 0)
      post(r: r >= 0) {
    return obj.value;
  }

  friend int friend_declared(FriendContracts obj) pre(obj.value >= 0);
};

int friend_declared(FriendContracts obj) {
  return obj.value;
}

inline int inline_contract(int x) pre(x >= 0);
inline int inline_contract(int x) {
  return x;
}

void use_friend_and_inline_contracts() {
  FriendContracts obj{1};
  friend_defined(obj);
  friend_declared(obj);
  inline_contract(1);
}
