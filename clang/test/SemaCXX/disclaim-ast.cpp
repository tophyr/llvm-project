// RUN: %clang_cc1 -fsyntax-only -std=c++20 -ast-dump %s | FileCheck %s

template<typename T>
T testTemplateDependent(T t) {
  return disclaim t;
}

// CHECK: FunctionDecl {{.*}} testTemplateDependent
// CHECK:   CXXDisclaimExpr
// CHECK:     DeclRefExpr {{.*}} 't'

struct Obj {
  Obj();
  ~Obj();
};

void testDisclaim() {
  Obj o;
  // CHECK: VarDecl {{.*}} o 'Obj' callinit destroyed
}

void testDisclaimRemoved() {
  Obj o;
  disclaim o;
  // CHECK: VarDecl {{.*}} o 'Obj' callinit{{$}}
}
