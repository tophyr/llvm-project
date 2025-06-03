// RUN: %clang_cc1 -fsyntax-only -std=c++20 -ast-dump %s | FileCheck %s

template<typename T>
T test_disclaim(T t) {
  return disclaim t;
}

// CHECK: FunctionDecl {{.*}} test_disclaim
// CHECK:   CXXDisclaimExpr
// CHECK:     DeclRefExpr {{.*}} 't'
