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
  void handle_contract_violation(const contract_violation&);
}

// P2900R14 §3.4.2: All entities referenced in a predicate are implicitly const-qualified

// Test 1: Assignment operators should be rejected
int global_var = 0;

void f1(int x) pre(x > 0);  // OK - read only
void f2(int x) pre((x = 0) == 0);  // expected-error {{contract predicate cannot modify variable}}
void f3(int x) pre((x += 1) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f4(int x) pre((x -= 1) >= 0);  // expected-error {{contract predicate cannot modify variable}}
void f5(int x) pre((x *= 2) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f6(int x) pre((x /= 2) >= 0);  // expected-error {{contract predicate cannot modify variable}}
void f7(int x) pre((x %= 2) == 0);  // expected-error {{contract predicate cannot modify variable}}
void f8(int x) pre((x &= 0xFF) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f9(int x) pre((x |= 1) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f10(int x) pre((x ^= 1) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f11(int x) pre((x <<= 1) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f12(int x) pre((x >>= 1) >= 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 2: Increment/decrement operators should be rejected
void f13(int x) pre(++x > 0);  // expected-error {{contract predicate cannot modify variable}}
void f14(int x) pre(x++ > 0);  // expected-error {{contract predicate cannot modify variable}}
void f15(int x) pre(--x >= 0);  // expected-error {{contract predicate cannot modify variable}}
void f16(int x) pre(x-- >= 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 3: Global variable modification should be rejected
void f17(int x) pre((global_var = x) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f18(int x) pre((global_var += x) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f19(int x) pre(++global_var > 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 4: Pointer dereference modification should be rejected
void f20(int* p) pre((*p = 5) > 0);  // expected-error {{contract predicate cannot modify variable}}
void f21(int* p) pre((++*p) > 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 5: Member function calls should check const-ness
struct S {
  int value;
  int getValue() const { return value; }
  bool setValueAndCheck(int v) { value = v; return value > 0; }
  S& increment() { ++value; return *this; }
};

void f22(const S& s) pre(s.getValue() > 0);  // OK - const member function
void f23(S& s) pre(s.setValueAndCheck(5));  // expected-error {{contract predicate cannot call non-const member function}}
void f24(S& s) pre(s.increment().getValue() > 0);  // expected-error {{contract predicate cannot call non-const member function}}

// Test 6: Post-conditions with non-const parameters trigger §3.4.4 first
int g1(int x) post(r: (x = 0) == 0);  // expected-error {{parameter 'x' used in a postcondition predicate must be const-qualified}}
int g2(int x) post(r: ++x > 0);  // expected-error {{parameter 'x' used in a postcondition predicate must be const-qualified}}

// Test 6b: Post-conditions with const parameters that try to modify trigger §3.4.2
int g3(const int x) post(r: (const_cast<int&>(x) = 0) == 0);  // expected-error {{contract predicate cannot modify variable}}
int g4(const int x) post(r: ++const_cast<int&>(x) > 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 7: Multiple conditions should all be checked
void h1(int x) pre(x > 0) pre((x = 1) > 0);  // expected-error {{contract predicate cannot modify variable}}
void h2(int x) pre((x = 1) > 0) pre(x > 0);  // expected-error {{contract predicate cannot modify variable}}

// Test 8: Nested expressions should be checked
void i1(int x) pre(((x = 0) == 0) && x > 0);  // expected-error {{contract predicate cannot modify variable}}
void i2(int x) pre(x > 0 ? (x = 1) > 0 : true);  // expected-error {{contract predicate cannot modify variable}}

// Test 9: Lambda in predicate should be checked
void j1(int x) pre([&x]() { x = 5; return true; }());  // expected-error {{contract predicate cannot modify variable}}
