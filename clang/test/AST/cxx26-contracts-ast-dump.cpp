// RUN: %clang_cc1 -std=c++2c -fcontracts -ast-dump %s | FileCheck %s

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

int divide(int a, int b) pre(b != 0);
// CHECK: FunctionDecl {{.*}} divide 'int (int, int)' {{.*}}contracts
// CHECK:   pre:
// CHECK:     BinaryOperator {{.*}} 'bool' '!='

int square(int x) post(r: r >= 0);
// CHECK: FunctionDecl {{.*}} square 'int (int)' {{.*}}contracts
// CHECK:   post:
// CHECK:     VarDecl {{.*}} implicit used r 'const int'
// CHECK:     BinaryOperator {{.*}} 'bool' '>='
// CHECK:       DeclRefExpr {{.*}} 'const int' lvalue Var {{.*}} 'r' 'const int'

int abs_val(int x) pre(x >= 0) pre(x < 1000) post(r: r >= 0);
// CHECK: FunctionDecl {{.*}} abs_val 'int (int)' {{.*}}contracts
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
