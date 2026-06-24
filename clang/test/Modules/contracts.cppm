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
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/B.cppm -emit-module-interface -o %t/B.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=enforce %t/UseB.cpp \
// RUN:   -fprebuilt-module-path=%t -emit-llvm -o - 2>&1 | FileCheck %t/UseB.cpp
//
// Header declarations with lambda contract predicates can be included by
// multiple modules and merged when those modules are imported together.
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/C.cppm \
// RUN:   -emit-module-interface -o %t/C.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/D.cppm \
// RUN:   -emit-module-interface -o %t/D.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts -I %t %t/UseCD.cpp \
// RUN:   -fprebuilt-module-path=%t -fsyntax-only -verify

// Test again with reduced BMI.
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/A.cppm -emit-reduced-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/Use.cpp -fprebuilt-module-path=%t \
// RUN:   -fsyntax-only -ast-dump-all 2>&1 | FileCheck %t/Use.cpp

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
  void handle_contract_violation(const contract_violation&);
}

export int divide(int a, int b) pre(b != 0);

export int clamp(const int x, const int lo, int hi)
    pre(lo <= hi)
    post(x >= lo);

export int square(int x) post(r: r >= 0);

//--- B.cppm
export module B;

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

export template <typename T>
T safe_add(const T a, const T b) pre(a > T{}) pre(b > T{}) post(r: r > a) {
  return a + b;
}

//--- Use.cpp
import A;

int test() {
  return divide(10, 2) + clamp(5, 0, 10) + square(3);
}

// CHECK: FunctionDecl {{.*}} divide 'int (int, int)' {{.*}}contracts
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '!='

// CHECK: FunctionDecl {{.*}} clamp 'int (const int, const int, int)' {{.*}}contracts
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} '<='
// CHECK:   post:
// CHECK:     BinaryOperator {{.*}} '>='

// CHECK: FunctionDecl {{.*}} square 'int (int)' {{.*}}contracts
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit {{.*}} r 'const int'
// CHECK:     BinaryOperator {{.*}} '>='
// CHECK:       DeclRefExpr {{.*}} 'const int' lvalue Var {{.*}} 'r' 'const int'

//--- UseB.cpp
import B;

int test_template() {
  return safe_add(1, 2);
}

// CHECK: define {{.*}} @_ZW1B8safe_add
// CHECK:   br i1 %{{.*}}, label %contract.cont, label %contract.handler
// CHECK: contract.handler:
// CHECK:   call void @abort

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
  void handle_contract_violation(const contract_violation&);
}

void f() pre([] { return true; }());

//--- C.cppm
module;
#include "HeaderWithLambdaContract.h"
export module C;
export using ::f;

//--- D.cppm
module;
#include "HeaderWithLambdaContract.h"
export module D;
export using ::f;

//--- UseCD.cpp
// expected-no-diagnostics
import C;
import D;

void use_header_lambda_contract() {
  f();
}
