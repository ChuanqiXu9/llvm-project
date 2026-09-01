// RUN: %clang_cc1 -std=c++2c -fcontracts -ast-dump %s | FileCheck %s

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

int divide(int a, int b) pre(b != 0);
// CHECK: FunctionDecl {{.*}} divide 'int (int, int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} 'bool' '!='

int square(int x) post(r: r >= 0);
// CHECK: FunctionDecl {{.*}} square 'int (int)'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit used r 'const int'
// CHECK:     BinaryOperator {{.*}} 'bool' '>='
// CHECK:       DeclRefExpr {{.*}} 'const int' lvalue Var {{.*}} 'r' 'const int'

int abs_val(int x) pre(x >= 0) pre(x < 1000) post(r: r >= 0);
// CHECK: FunctionDecl {{.*}} abs_val 'int (int)'
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} 'bool' '>='
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} 'bool' '<'
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit used r 'const int'
// CHECK:     BinaryOperator {{.*}} 'bool' '>='

void f(int x) {
  contract_assert(x > 0);
}
// CHECK: FunctionDecl {{.*}} f 'void (int)'
// CHECK:   CompoundStmt
// CHECK:     ContractAssertStmt
// CHECK:       BinaryOperator {{.*}} 'bool' '>'
