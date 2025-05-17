// RUN: %clang_cc1 -fsyntax-only -std=c++20 -ast-dump %s | FileCheck %s

template<typename T>
T test_moov(T t) {
  return moov t;
}

// CHECK: FunctionDecl {{.*}} test_moov
// CHECK:   CXXMoovExpr
// CHECK:     DeclRefExpr {{.*}} 't'
