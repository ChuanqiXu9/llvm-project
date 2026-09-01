// Tests that contract annotations survive module serialization/deserialization,
// and that semantic checks and CodeGen work across module boundaries.
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// AST round-trip test.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/A.cppm -emit-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/Use.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/Use.cpp
//
// Template instantiation and CodeGen across modules.
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcxx-exceptions -fexceptions \
// RUN:   %t/B.cppm -emit-module-interface -o %t/B.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   -fcxx-exceptions -fexceptions %t/UseB.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseB.cpp
//
// Complex module serialization with concepts, class templates, member templates,
// nested namespaces, and operators.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/E.cppm -emit-module-interface -o %t/E.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseE.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseE.cpp --check-prefix=E
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/UseECodeGen.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseECodeGen.cpp --check-prefix=ECG
//
// Module partitions and re-export chains with contracts.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/GPart.cppm -emit-module-interface -o %t/G-Contracts.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/G.cppm -emit-module-interface \
// RUN:   -fmodule-file=G:Contracts=%t/G-Contracts.pcm -o %t/G.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/H.cppm -emit-module-interface \
// RUN:   -fprebuilt-module-path=%t -o %t/H.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseGH.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseGH.cpp --check-prefix=GH
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/UseGHCodeGen.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseGHCodeGen.cpp --check-prefix=GHCG
//
// Multiple partitions, explicit specializations, a private module fragment, and
// an implementation unit with contracts.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/ITypes.cppm -emit-module-interface -o %t/I-Types.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/IAlgo.cppm -emit-module-interface \
// RUN:   -fmodule-file=I:Types=%t/I-Types.pcm -o %t/I-Algo.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/I.cppm -emit-module-interface \
// RUN:   -fmodule-file=I:Types=%t/I-Types.pcm -fmodule-file=I:Algo=%t/I-Algo.pcm -o %t/I.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseI.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseI.cpp --check-prefix=I
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/UseICodeGen.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseICodeGen.cpp --check-prefix=ICG
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/I-impl.cpp \
// RUN:   -fmodule-file=I=%t/I.pcm -fmodule-file=I:Types=%t/I-Types.pcm \
// RUN:   -fmodule-file=I:Algo=%t/I-Algo.pcm -emit-llvm -o - 2>&1 | FileCheck %t/I-impl.cpp --check-prefix=IMPLCG
//
// Extern templates, friend contracts, lambda predicates, and noexcept across a
// module interface and implementation unit.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/J.cppm -emit-module-interface -o %t/J.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseJ.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseJ.cpp --check-prefix=J
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/UseJCodeGen.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseJCodeGen.cpp --check-prefix=JCG
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/J-impl.cpp \
// RUN:   -fmodule-file=J=%t/J.pcm -emit-llvm -o - 2>&1 | FileCheck %t/J-impl.cpp --check-prefix=JIMPLCG
//
// Header declarations with lambda contract predicates can be included by
// multiple modules and merged when those modules are imported together.
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/C.cppm \
// RUN:   -emit-module-interface -o %t/C.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/D.cppm \
// RUN:   -emit-module-interface -o %t/D.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/UseCD.cpp \
// RUN:   -fprebuilt-module-path=%t -fsyntax-only -verify
//
// PCH serialization/deserialization with contracts.
// RUN: %clang_cc1 -std=c++2c -fcontracts -x c++-header %t/PCH.h \
// RUN:   -emit-pch -o %t/PCH.pch
// RUN: %clang_cc1 -std=c++2c -fcontracts -include-pch %t/PCH.pch %t/UsePCH.cpp \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UsePCH.cpp --check-prefix=PCH
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   -include-pch %t/PCH.pch %t/UsePCHCodeGen.cpp -emit-llvm -o - 2>&1 \
// RUN:   | FileCheck %t/UsePCHCodeGen.cpp --check-prefix=PCHCG
//
// A contract must be able to synthesize its source location when the standard
// library's private source_location implementation was imported from a BMI.
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/PrivateSourceLocation.cppm \
// RUN:   -emit-module-interface -o %t/PrivateSourceLocation.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   %t/UsePrivateSourceLocation.cpp \
// RUN:   -fmodule-file=PrivateSourceLocation=%t/PrivateSourceLocation.pcm \
// RUN:   -fsyntax-only
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   %t/DefaultHandler.cppm \
// RUN:   -fmodule-file=PrivateSourceLocation=%t/PrivateSourceLocation.pcm \
// RUN:   -emit-llvm -o - 2>&1 | FileCheck %t/DefaultHandler.cppm

// Test again with reduced BMI.
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/A.cppm -emit-reduced-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/Use.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/Use.cpp
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/E.cppm -emit-reduced-module-interface -o %t/E.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseE.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseE.cpp --check-prefix=E
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/GPart.cppm -emit-reduced-module-interface -o %t/G-Contracts.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/G.cppm -emit-reduced-module-interface \
// RUN:   -fmodule-file=G:Contracts=%t/G-Contracts.pcm -o %t/G.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/H.cppm -emit-reduced-module-interface \
// RUN:   -fprebuilt-module-path=%t -o %t/H.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseGH.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseGH.cpp --check-prefix=GH
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/ITypes.cppm -emit-reduced-module-interface -o %t/I-Types.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/IAlgo.cppm -emit-reduced-module-interface \
// RUN:   -fmodule-file=I:Types=%t/I-Types.pcm -o %t/I-Algo.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/I.cppm -emit-reduced-module-interface \
// RUN:   -fmodule-file=I:Types=%t/I-Types.pcm -fmodule-file=I:Algo=%t/I-Algo.pcm -o %t/I.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseI.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseI.cpp --check-prefix=I
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/J.cppm -emit-reduced-module-interface -o %t/J.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/UseJ.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/UseJ.cpp --check-prefix=J
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/PrivateSourceLocation.cppm \
// RUN:   -emit-reduced-module-interface -o %t/PrivateSourceLocation.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce \
// RUN:   %t/UsePrivateSourceLocation.cpp \
// RUN:   -fmodule-file=PrivateSourceLocation=%t/PrivateSourceLocation.pcm \
// RUN:   -fsyntax-only

//--- A.cppm
export module A;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export int divide(int a, int b) pre(b != 0);

export int clamp(const int x, const int lo, int hi)
    pre(lo <= hi)
    post(x >= lo);

export int square(int x) post(r: r >= 0);

export int bounded(const int x) pre(x > 0) post(r: r >= x);

export auto exported_auto() post(r: r > 0) {
  return 1;
}

export struct Counter {
  int value;

  int get() const post(r: r >= value) {
    return value;
  }

  int add(const int delta) const pre(delta >= 0) post(r: r >= value) {
    return value + delta;
  }
};

export inline int inline_checked(const int x) pre(x > 0) post(r: r >= x) {
  return x;
}

//--- B.cppm
export module B;

export namespace std {
  struct source_location {
    struct __impl {
      const char* _M_file_name;
      const char* _M_function_name;
      unsigned _M_line;
      unsigned _M_column;
    };
  };
}

export namespace std::contracts {
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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export template <typename T>
T safe_add(const T a, const T b) pre(a > T{}) pre(b > T{}) post(r: r > a) {
  return a + b;
}

export template <int N>
int nttp_value() pre(N > 0) post(r: r == N) {
  return N;
}

export template <typename T>
T module_identity(const T x) pre(x > T{}) post(r: r >= x) {
  return x;
}

export template <typename T>
T module_asserted(const T x) {
  contract_assert(x > T{});
  return x;
}

export struct ModuleCallable {
  int value;

  int operator()(const int scale) const pre(scale > 0) post(r: r >= value) {
    return value * scale;
  }
};

export struct PredicateTemporary {
  ~PredicateTemporary();
  bool valid() const;
};

export struct TemporaryResult {
  TemporaryResult();
  ~TemporaryResult();
  PredicateTemporary predicate() const;
};

export template <typename T>
T make_temporary_result()
    post(result: result.predicate().valid()) {
  return T();
}

//--- E.cppm
export module E;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export namespace complex {
inline namespace v1 {

template <typename T>
concept Positive = requires(T value) { value > T{}; };

template <typename T>
  requires Positive<T>
T constrained_identity(const T x) pre(x > T{}) post(r: r >= x) {
  return x;
}

template <typename T, int Bias>
struct Accumulator {
  T base;

  Accumulator(const T base) pre(base > T{}) post(this->base >= T{})
      : base(base) {}

  T member_add(const T x) const pre(x >= T{}) post(r: r >= base) {
    return base + x + static_cast<T>(Bias);
  }

  template <typename U>
  U member_convert(const U value) const pre(value >= U{}) post(r: r >= value) {
    return value;
  }

  T operator()(const T factor) const pre(factor > T{}) post(r: r >= base) {
    return base * factor;
  }
};

template <typename T, int Bias>
Accumulator<T, Bias> make_accumulator(const T base)
    pre(base > T{})
    post(r: r.base >= T{}) {
  return {base};
}

template <typename T, int Bias>
T accumulate(const Accumulator<T, Bias> &acc, const T x)
    pre(x >= T{})
    post(r: r >= acc.base) {
  return acc.base + x + static_cast<T>(Bias);
}

template <typename T, typename U>
U convert_value(const T &, const U value)
    pre(value >= U{})
    post(r: r >= value) {
  return value;
}

struct ComplexCallable {
  int value;

  int operator()(const int factor) const pre(factor > 0) post(r: r >= value) {
    return value * factor;
  }
};

int namespace_fn(const int x) pre(x > 0) post(r: r >= x) {
  return x;
}

} // namespace v1
} // namespace complex

//--- Use.cpp
import A;

int test() {
  Counter c{4};
  return divide(10, 2) + clamp(5, 0, 10) + square(3) + bounded(4) +
         exported_auto() + c.get() + c.add(1) + inline_checked(2);
}

// CHECK: FunctionDecl {{.*}} divide 'int (int, int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '!='

// CHECK: FunctionDecl {{.*}} clamp 'int (const int, const int, int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '<='
// CHECK:   post:
// CHECK:     BinaryOperator {{.*}} '>='

// CHECK: FunctionDecl {{.*}} square 'int (int)'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// CHECK:     BinaryOperator {{.*}} '>='
// CHECK:       DeclRefExpr {{.*}} 'const int' lvalue Var {{.*}} 'r' 'const int'

// CHECK: FunctionDecl {{.*}} bounded 'int (const int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '>'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// CHECK:     BinaryOperator {{.*}} '>='

// CHECK: FunctionDecl {{.*}} exported_auto 'int ()'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'

// CHECK: CXXRecordDecl {{.*}} Counter
// CHECK: CXXMethodDecl {{.*}} get 'int () const'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// CHECK: CXXMethodDecl {{.*}} add 'int (const int) const'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '>='
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'

// CHECK: FunctionDecl {{.*}} inline_checked 'int (const int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '>'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'

//--- UseB.cpp
import B;

extern "C++" void
handle_contract_violation(const std::contracts::contract_violation&);

int test_template() {
  ModuleCallable callable{3};
  return safe_add(1, 2) + nttp_value<3>() + module_identity(4) +
         module_asserted(5) + callable(2);
}

TemporaryResult test_temporary_cleanup() {
  return make_temporary_result<TemporaryResult>();
}

// CHECK: define {{.*}} @_ZW1B8safe_add
// CHECK:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @abort
// CHECK-LABEL: define linkonce_odr {{.*}}module_asserted
// CHECK:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @abort
// CHECK-LABEL: define linkonce_odr {{.*}}make_temporary_result
// CHECK: invoke {{.*}}PredicateTemporary{{.*}}valid
// CHECK: call void {{.*}}PredicateTemporary{{.*}}D1Ev
// CHECK-NEXT: br i1
// CHECK: landingpad
// CHECK: call void {{.*}}PredicateTemporary{{.*}}D1Ev

//--- UseE.cpp
import E;

int use_complex_module_ast() {
  auto acc = complex::make_accumulator<int, 2>(1);
  complex::ComplexCallable callable{3};
  return complex::constrained_identity(3) + complex::accumulate<int, 2>(acc, 4) +
         complex::convert_value(acc, 5) + acc.member_add(1) +
         acc.member_convert<int>(7) + acc(2) + callable(2) +
         complex::namespace_fn(6);
}

// E: FunctionTemplateDecl {{.*}} constrained_identity
// E: FunctionDecl {{.*}} constrained_identity 'T (const T)'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// E: CXXRecordDecl {{.*}} implicit struct Accumulator
// E: CXXConstructorDecl {{.*}} Accumulator 'void (const int)'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     CXXThisExpr
// E: CXXMethodDecl {{.*}} member_add 'int (const int) const'
// E:   pre:
// E:     BinaryOperator {{.*}} '>='
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// E: FunctionTemplateDecl {{.*}} member_convert
// E: CXXMethodDecl {{.*}} operator() 'int (const int) const'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// E: FunctionTemplateDecl {{.*}} make_accumulator
// E: FunctionDecl {{.*}} make_accumulator 'Accumulator<T, Bias> (const T)'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const Accumulator<int, 2>'
// E: FunctionTemplateDecl {{.*}} accumulate
// E: FunctionDecl {{.*}} accumulate 'T (const Accumulator<T, Bias> &, const T)'
// E:   pre:
// E:     BinaryOperator {{.*}} '>='
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// E: FunctionTemplateDecl {{.*}} convert_value
// E: CXXRecordDecl {{.*}} ComplexCallable
// E: CXXMethodDecl {{.*}} operator() 'int (const int) const'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// E: FunctionDecl {{.*}} namespace_fn 'int (const int)'
// E:   pre:
// E:     BinaryOperator {{.*}} '>'
// E:   post:
// E:     VarDecl {{.*}} implicit {{.*}} r 'const int'

//--- UseECodeGen.cpp
import E;

int use_complex_module_codegen() {
  auto acc = complex::make_accumulator<int, 2>(1);
  complex::ComplexCallable callable{3};
  return complex::constrained_identity(3) + complex::accumulate<int, 2>(acc, 4) +
         complex::convert_value(acc, 5) + acc.member_add(1) +
         acc.member_convert<int>(7) + acc(2) + callable(2) +
         complex::namespace_fn(6);
}

// ECG-LABEL: define {{.*}}use_complex_module_codegen
// ECG: contract.handler:
// ECG: call void @abort

//--- GPart.cppm
export module G:Contracts;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export namespace partitioned {

template <int Limit>
struct Guard {
  int value;

  int clamp(const int x) const pre(x >= 0) post(r: r >= 0) {
    return x < Limit ? x : Limit;
  }

  template <int Bias>
  int biased(const int x) const pre(Bias >= 0) pre(x >= 0) post(r: r >= value) {
    return value + x + Bias;
  }
};

template <typename T>
T part_identity(const T x) pre(x > T{}) post(r: r >= x) {
  return x;
}

inline int part_inline(const int x) pre(x > 0) post(r: r >= x) {
  return x;
}

} // namespace partitioned

//--- G.cppm
export module G;
export import :Contracts;

export namespace partitioned {

template <int Bias>
int primary_wrap(const Guard<Bias> &guard, const int x)
    pre(x >= 0)
    post(r: r >= guard.value) {
  return guard.template biased<1>(x);
}

struct PrimaryCallable {
  int base;

  int operator()(const int x) const pre(x >= 0) post(r: r >= base) {
    return base + x;
  }
};

} // namespace partitioned

//--- H.cppm
export module H;
export import G;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export namespace reexported {

inline int through_reexport(const int x) pre(x > 0) post(r: r >= x) {
  partitioned::Guard<8> guard{x};
  return partitioned::primary_wrap<8>(guard, x);
}

template <typename T>
T reexport_identity(const T x) pre(x > T{}) post(r: r >= x) {
  return partitioned::part_identity(x);
}

} // namespace reexported

//--- UseGH.cpp
import H;

int use_partition_reexport_ast() {
  partitioned::Guard<8> guard{3};
  partitioned::PrimaryCallable callable{4};
  return guard.clamp(9) + guard.biased<2>(1) + partitioned::part_inline(2) +
         partitioned::primary_wrap<8>(guard, 1) + callable(2) +
         reexported::through_reexport(3) + reexported::reexport_identity(5);
}

// GH: ClassTemplateDecl {{.*}} Guard
// GH: CXXMethodDecl {{.*}} clamp 'int (const int) const'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>='
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// GH: FunctionTemplateDecl {{.*}} biased
// GH: FunctionTemplateDecl {{.*}} part_identity
// GH: FunctionDecl {{.*}} part_identity 'T (const T)'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>'
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// GH: FunctionDecl {{.*}} part_inline 'int (const int)'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>'
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// GH: FunctionTemplateDecl {{.*}} primary_wrap
// GH: FunctionDecl {{.*}} primary_wrap 'int (const Guard<Bias> &, const int)'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>='
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// GH: CXXRecordDecl {{.*}} PrimaryCallable
// GH: CXXMethodDecl {{.*}} operator() 'int (const int) const'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>='
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// GH: FunctionDecl {{.*}} through_reexport 'int (const int)'
// GH:   pre:
// GH:     BinaryOperator {{.*}} '>'
// GH:   post:
// GH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// GH: FunctionTemplateDecl {{.*}} reexport_identity

//--- UseGHCodeGen.cpp
import H;

int use_partition_reexport_codegen() {
  partitioned::Guard<8> guard{3};
  partitioned::PrimaryCallable callable{4};
  return guard.clamp(9) + guard.biased<2>(1) + partitioned::part_inline(2) +
         partitioned::primary_wrap<8>(guard, 1) + callable(2) +
         reexported::through_reexport(3) + reexported::reexport_identity(5);
}

// GHCG-LABEL: define {{.*}}use_partition_reexport_codegen
// GHCG: contract.handler:
// GHCG: call void @abort

//--- ITypes.cppm
export module I:Types;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export namespace advanced {

template <typename T>
concept NonNegative = requires(T value) { value >= T{}; };

template <typename T>
struct Box {
  T value;

  T get() const post(r: r >= T{}) {
    return value;
  }

  template <int Scale>
  T scaled(const T x) const pre(Scale > 0) pre(x >= T{}) post(r: r >= value) {
    return value + x * static_cast<T>(Scale);
  }
};

template <typename T>
T choose(const Box<T> &box, const T fallback)
    pre(fallback >= T{})
    post(r: r >= fallback) {
  return box.value > fallback ? box.value : fallback;
}

template <typename T>
struct Selector {
  static int id(const int x) pre(x >= 0) post(r: r >= x) {
    return x;
  }
};

template <>
struct Selector<int> {
  static int id(const int x) pre(x > 0) post(r: r == x) {
    return x;
  }
};

} // namespace advanced

//--- IAlgo.cppm
export module I:Algo;
import :Types;

export namespace advanced {

template <typename T>
  requires NonNegative<T>
T combine(const Box<T> &lhs, const Box<T> &rhs)
    pre(lhs.value >= T{})
    pre(rhs.value >= T{})
    post(r: r >= lhs.value) {
  return lhs.value + rhs.value;
}

struct Runner {
  template <typename T>
  T operator()(const Box<T> &box, const T increment) const
      pre(increment >= T{})
      post(r: r >= box.value) {
    return box.value + increment;
  }
};

} // namespace advanced

//--- I.cppm
export module I;
export import :Types;
export import :Algo;

namespace advanced {
int hidden_adjust(const int x) pre(x > 0) post(r: r > x);
}

export namespace advanced {

inline int public_entry(const int x) pre(x > 0) post(r: r >= x) {
  Box<int> box{x};
  Runner run;
  return run(box, 1) + Selector<int>::id(x);
}

template <typename T>
T public_template(const T x) pre(x > T{}) post(r: r >= x) {
  Box<T> box{x};
  return combine(box, box);
}

inline int private_backed(const int x) pre(x > 0) post(r: r > x) {
  return hidden_adjust(x);
}

int impl_defined(const int x) pre(x > 0) post(r: r >= x);

} // namespace advanced

module :private;

namespace advanced {

int hidden_adjust(const int x) {
  return x + 1;
}

} // namespace advanced

//--- I-impl.cpp
module I;

namespace advanced {

int impl_defined(const int x) {
  return public_entry(x);
}

} // namespace advanced

int use_impl_unit_codegen() {
  return advanced::impl_defined(2);
}

// IMPLCG-LABEL: define {{.*}}use_impl_unit_codegen
// IMPLCG: contract.handler:
// IMPLCG: call void @abort

//--- UseI.cpp
import I;

int use_advanced_ast() {
  advanced::Box<int> box{4};
  advanced::Runner run;
  return box.get() + box.scaled<2>(1) + advanced::choose(box, 3) +
         advanced::Selector<int>::id(5) + advanced::public_entry(2) +
         advanced::public_template(3) + run(box, 1) +
         advanced::private_backed(2) + advanced::impl_defined(2);
}

// I: ClassTemplateDecl {{.*}} Box
// I: CXXMethodDecl {{.*}} get 'T () const'
// I:   post:
// I:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// I: FunctionTemplateDecl {{.*}} scaled
// I: FunctionTemplateDecl {{.*}} choose
// I: FunctionDecl {{.*}} choose 'T (const Box<T> &, const T)'
// I:   pre:
// I:     BinaryOperator {{.*}} '>='
// I:   post:
// I:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// I: ClassTemplateDecl {{.*}} Selector
// I: ClassTemplateSpecializationDecl {{.*}} Selector definition
// I: CXXMethodDecl {{.*}} id 'int (const int)' static
// I: FunctionTemplateDecl {{.*}} combine
// I: CXXRecordDecl {{.*}} Runner
// I: FunctionTemplateDecl {{.*}} operator()
// I: FunctionDecl {{.*}} public_entry 'int (const int)'
// I:   pre:
// I:     BinaryOperator {{.*}} '>'
// I:   post:
// I:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// I: FunctionTemplateDecl {{.*}} public_template
// I: FunctionDecl {{.*}} private_backed 'int (const int)'
// I:   post:
// I:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// I: FunctionDecl {{.*}} impl_defined 'int (const int)'
// I:   post:
// I:     VarDecl {{.*}} implicit {{.*}} r 'const int'

//--- UseICodeGen.cpp
import I;

int use_advanced_codegen() {
  advanced::Box<int> box{4};
  advanced::Runner run;
  return box.get() + box.scaled<2>(1) + advanced::choose(box, 3) +
         advanced::Selector<int>::id(5) + advanced::public_entry(2) +
         advanced::public_template(3) + run(box, 1) +
         advanced::private_backed(2);
}

// ICG-LABEL: define {{.*}}use_advanced_codegen
// ICG: contract.handler:
// ICG: call void @abort

//--- J.cppm
export module J;

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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

export namespace edge {

template <typename T>
T externalized(const T x) pre(x > T{}) post(r: r >= x) {
  return x;
}

extern template int externalized<int>(const int);

struct FriendBox {
  int value;

  friend int friend_value(const FriendBox &box)
      pre(box.value > 0)
      post(r: r >= box.value) {
    return box.value;
  }

  template <typename T>
  friend T friend_template(const FriendBox &box, const T extra)
      pre(extra >= T{})
      post(r: r >= extra) {
    return static_cast<T>(box.value) + extra;
  }
};

inline int lambda_checked(const int x)
    pre(([x] { return x > 0; })())
    post(r: ([x](const int y) { return y >= x; })(r)) {
  return x;
}

inline int lambda_by_ref_checked(const int x)
    pre(([&x] { return x > 0; })()) {
  return x;
}

inline int lambda_implicit_copy_checked(const int x)
    pre(([=] { return x > 0; })()) {
  return x;
}

inline int lambda_implicit_ref_checked(const int x)
    pre(([&] { return x > 0; })()) {
  return x;
}

inline int noexcept_checked(const int x) noexcept pre(x > 0) post(r: r >= x) {
  return x;
}

} // namespace edge

//--- J-impl.cpp
module J;

template int edge::externalized<int>(const int);

int use_j_impl_codegen() {
  return edge::externalized<int>(3);
}

// JIMPLCG-LABEL: define {{.*}}use_j_impl_codegen
// JIMPLCG: call {{.*}}externalized

//--- UseJ.cpp
import J;

int use_edge_ast() {
  edge::FriendBox box{3};
  return edge::externalized(2) + friend_value(box) + friend_template<int>(box, 4) +
         edge::lambda_checked(5) + edge::lambda_by_ref_checked(6) +
         edge::lambda_implicit_copy_checked(7) +
         edge::lambda_implicit_ref_checked(8) + edge::noexcept_checked(9);
}

// J: FunctionTemplateDecl {{.*}} externalized
// J: FunctionDecl {{.*}} externalized 'T (const T)'
// J:   pre:
// J:     BinaryOperator {{.*}} '>'
// J:   post:
// J:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// J: CXXRecordDecl {{.*}} FriendBox definition
// J: FunctionDecl {{.*}} friend_value 'int (const FriendBox &)'
// J:   pre:
// J:     BinaryOperator {{.*}} '>'
// J:   post:
// J:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// J: FunctionTemplateDecl {{.*}} friend_template
// J: FunctionDecl {{.*}} lambda_checked 'int (const int)'
// J:   pre:
// J:     LambdaExpr
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'x' 'const int' refers_to_enclosing_variable_or_capture
// J:       FieldDecl {{.*}} implicit {{.*}} 'const int'
// J:   post:
// J:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// J:     LambdaExpr
// J:       ParmVarDecl {{.*}} y 'const int'
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'y' 'const int'
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'x' 'const int' refers_to_enclosing_variable_or_capture
// J:       FieldDecl {{.*}} implicit {{.*}} 'const int'
// J: FunctionDecl {{.*}} lambda_by_ref_checked 'int (const int)'
// J:   pre:
// J:     LambdaExpr
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'x' 'const int' refers_to_enclosing_variable_or_capture
// J:       FieldDecl {{.*}} implicit {{.*}} 'const int &'
// J: FunctionDecl {{.*}} lambda_implicit_copy_checked 'int (const int)'
// J:   pre:
// J:     LambdaExpr
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'x' 'const int' refers_to_enclosing_variable_or_capture
// J:       FieldDecl {{.*}} implicit {{.*}} 'const int'
// J: FunctionDecl {{.*}} lambda_implicit_ref_checked 'int (const int)'
// J:   pre:
// J:     LambdaExpr
// J:       DeclRefExpr {{.*}} ParmVar {{.*}} 'x' 'const int' refers_to_enclosing_variable_or_capture
// J:       FieldDecl {{.*}} implicit {{.*}} 'const int &'
// J: FunctionDecl {{.*}} noexcept_checked 'int (const int) noexcept'
// J:   pre:
// J:     BinaryOperator {{.*}} '>'
// J:   post:
// J:     VarDecl {{.*}} implicit {{.*}} r 'const int'

//--- UseJCodeGen.cpp
import J;

int use_edge_codegen() {
  edge::FriendBox box{3};
  return friend_value(box) + friend_template<int>(box, 4) +
         edge::lambda_checked(5) + edge::lambda_by_ref_checked(6) +
         edge::lambda_implicit_copy_checked(7) +
         edge::lambda_implicit_ref_checked(8) + edge::noexcept_checked(9);
}

// JCG-LABEL: define {{.*}}use_edge_codegen
// JCG: contract.handler:
// JCG: call void @abort
// JCG-LABEL: define linkonce_odr {{.*}}lambda_by_ref_checked
// JCG: %[[BYREF_X:.*]] = alloca i32
// JCG: %[[BYREF_CAPTURE:.*]] = getelementptr {{.*}}
// JCG: store ptr %[[BYREF_X]], ptr %[[BYREF_CAPTURE]]
// JCG-LABEL: define linkonce_odr {{.*}}lambda_implicit_copy_checked
// JCG: %[[IMPLICIT_COPY_X:.*]] = alloca i32
// JCG: %[[IMPLICIT_COPY_CAPTURE:.*]] = getelementptr {{.*}}
// JCG: %[[IMPLICIT_COPY_VALUE:.*]] = load i32, ptr %[[IMPLICIT_COPY_X]]
// JCG: store i32 %[[IMPLICIT_COPY_VALUE]], ptr %[[IMPLICIT_COPY_CAPTURE]]
// JCG-LABEL: define linkonce_odr {{.*}}lambda_implicit_ref_checked
// JCG: %[[IMPLICIT_REF_X:.*]] = alloca i32
// JCG: %[[IMPLICIT_REF_CAPTURE:.*]] = getelementptr {{.*}}
// JCG: store ptr %[[IMPLICIT_REF_X]], ptr %[[IMPLICIT_REF_CAPTURE]]

//--- HeaderWithLambdaContract.h
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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

void f() pre([] { return true; }());
int g(const int x) pre(([x] { return x > 0; })()) post(r: r == x);
inline int h(const int x) pre(x > 0) post(r: r >= x) { return x; }

//--- C.cppm
module;
#include "HeaderWithLambdaContract.h"
export module C;
export using ::f;
export using ::g;
export using ::h;

//--- D.cppm
module;
#include "HeaderWithLambdaContract.h"
export module D;
export using ::f;
export using ::g;
export using ::h;

//--- UseCD.cpp
// expected-no-diagnostics
import C;
import D;

void use_header_lambda_contract() {
  f();
  (void)g(1);
  h(2);
}

//--- PCH.h
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
extern "C++" void handle_contract_violation(const std::contracts::contract_violation&);

int pch_pre(int x) pre(x > 0);
int pch_post(const int x) post(r: r >= x);

inline int pch_inline(const int x) pre(x > 0) post(r: r >= x) {
  return x;
}

template <typename T>
T pch_template(const T x) pre(x > T{}) post(r: r >= x) {
  return x;
}

struct PCHBox {
  int value;

  int get() const post(r: r >= value) {
    return value;
  }

  int add(const int delta) const pre(delta >= 0) post(r: r >= value) {
    return value + delta;
  }
};

//--- UsePCH.cpp
int use_pch_ast() {
  PCHBox box{3};
  return pch_pre(1) + pch_post(2) + pch_inline(3) +
         pch_template(4) + box.get() + box.add(1);
}

// PCH: FunctionDecl {{.*}} pch_pre 'int (int)'
// PCH:   pre:
// PCH:     BinaryOperator {{.*}} '>'
// PCH: FunctionDecl {{.*}} pch_post 'int (const int)'
// PCH:   post:
// PCH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// PCH: FunctionDecl {{.*}} pch_inline 'int (const int)'
// PCH:   pre:
// PCH:     BinaryOperator {{.*}} '>'
// PCH:   post:
// PCH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// PCH: FunctionTemplateDecl {{.*}} pch_template
// PCH: FunctionDecl {{.*}} pch_template 'T (const T)'
// PCH:   pre:
// PCH:     BinaryOperator {{.*}} '>'
// PCH:   post:
// PCH:     VarDecl {{.*}} implicit {{.*}} r 'const T'
// PCH: CXXRecordDecl {{.*}} PCHBox
// PCH: CXXMethodDecl {{.*}} get 'int () const'
// PCH:   post:
// PCH:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// PCH: CXXMethodDecl {{.*}} add 'int (const int) const'
// PCH:   pre:
// PCH:     BinaryOperator {{.*}} '>='
// PCH:   post:
// PCH:     VarDecl {{.*}} implicit {{.*}} r 'const int'

//--- UsePCHCodeGen.cpp
int use_pch_codegen() {
  PCHBox box{3};
  return pch_inline(1) + pch_template(2) + box.get() + box.add(1);
}

// PCHCG-LABEL: define {{.*}}use_pch_codegen
// PCHCG: contract.handler:
// PCHCG: call void @abort

//--- PrivateSourceLocation.cppm
export module PrivateSourceLocation;

export namespace std {
namespace contracts {
class contract_violation;
}

class source_location {
  struct __impl {
    const char* _M_file_name;
    const char* _M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };

  friend class contracts::contract_violation;
};
}

export namespace std::contracts {
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

//--- UsePrivateSourceLocation.cpp
import PrivateSourceLocation;

void handle_contract_violation(
    const std::contracts::contract_violation&);

int private_source_location(const int value)
    post(result : result == value) {
  return value;
}

//--- DefaultHandler.cppm
export module DefaultHandler;

import PrivateSourceLocation;

export int default_handler(const int value) pre(value > 0) {
  return value;
}

export int after_default_handler() { return 0; }

// CHECK-NOT: @_ZW{{.*}}25handle_contract_violation
// CHECK: call void @_Z25handle_contract_violation
// CHECK: define{{.*}} @_ZW14DefaultHandler21after_default_handlerv
// CHECK-NOT: @_ZW{{.*}}25handle_contract_violation
