// RUN: %clang_cc1 -fexceptions -fcxx-exceptions -fsyntax-only -verify %s

struct MoveOnly {
  constexpr MoveOnly(int val)
    : val_{val} {}
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  
  int val_;
};

struct Immovable : MoveOnly {
  using MoveOnly::MoveOnly;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveObly& operator=(MoveOnly&&) = delete;
};

struct OnlyDisclaimable : Immovable {
  using Immovable::Immovable;
  operator disclaim(OnlyDisclaimable& dest, OnlyDisclaimable const& src);
};

void testIsMoveLike() {
  MoveOnly a{5};
  MoveOnly b = disclaim a;
}

void testNoExpression() {
  disclaim;                     // expected-error {{expected expression}}
}

void testNonLValues() {
  int a = 42;

  disclaim 17;                  // expected-error {{can only disclaim local variables}}
  disclaim (a * 17);            // expected-error {{can only disclaim local variables}}
  disclaim MoveOnly{123};       // expected-error {{can only disclaim local variables}}
}

void testLocals() {
  constexpr int a = 42;
  const int b = a + 22;
  MoveOnly c{123};

  auto _a = disclaim a;         // expected-error {{can only disclaim variables with automatic duration; no globals, local-statics or constexpr}}
  auto _b = disclaim b;
  auto _c = disclaim c;
}

int testParameter(int a) {
  return disclaim a;
}

int testSubobject() {
  MoveOnly c{123};

  disclaim c.val_;              // expected-error {{can only disclaim local variables}}
}

void testReturnVal() {
  auto d = []() -> int& {
    static int d_ = 256;
    return d_;
  };

  disclaim d();                 // expected-error {{can only disclaim local variables}}
}

void testWrongScope() {
  int a = 42;
  {
    disclaim a;                 // expected-error {{cannot disclaim a variable not defined in the same scope}}
  }
}

void testLocalStatic() {
  static int a = 42;
  disclaim a;                   // expected-error {{can only disclaim variables with automatic duration; no globals, local-statics or constexpr}}
}

namespace testGlobal {

int g_a = 42;
int g_b = disclaim g_a;         // expected-error {{can only disclaim variables with automatic duration; no globals, local-statics or constexpr}}

}

void testDisclaimInLambdaCapture() {
  int a = 42;
  auto l = [b = disclaim a] {
    return b;
  };
}

void testDisclaimingLambdaCapture() {
  auto l = [a = 42] {
    disclaim a;                 // expected-error {{cannot disclaim lambda captures}}
  };
}

void testNestedLambdas() {
  int x = 5;
  auto l = [=]() {
    auto l2 = [=]() {
      disclaim x;               // expected-error {{cannot disclaim lambda captures}}
    };
  };
}

int testComma() {
  int a = 1, b = 2;
  return (a, disclaim b);       // expected-warning {{left operand of comma operator has no effect}}
}

int testTernary(bool cond) {
  int a = 1, b = 2;
  return cond ? disclaim a : b;
}

void testLoopVar() {
  for (int i = 0; i < 10; ++i) {
    disclaim i;                 // expected-error {{cannot disclaim a variable not defined in the same scope}}
  }
}

void testCtorInitializer() {
  struct Foo {
    Foo(int val)
      : val_(disclaim val) {}

    int val_;
  };

  Foo foo{42};
}

void testArrayMember() {
  int arr[2];
  disclaim arr[0];              // expected-error {{can only disclaim local variables}}
}

void testUseAfterDisclaim() {
  MoveOnly a{42};
  auto b = disclaim a;          // expected-note {{disclaimed here}}
  b.val_ += a.val_;             // expected-error {{use of disclaimed identifier 'a'}}
}

void testUnsequencedUse1() {
  int a = 42;
  auto l = [](int x, int y) {};
  l(a, disclaim a);             // expected-error {{cannot use 'a' and disclaim it in the same expression}} expected-note {{disclaimed here}}
}

void testUnsequencedUse2() {
  int a = 42;
  auto l = [](int x, int y) {};
  l(disclaim a, a);             // expected-error {{cannot use 'a' and disclaim it in the same expression}} expected-error {{use of disclaimed identifier 'a'}} expected-note {{disclaimed here}} expected-note {{disclaimed here}}
}

void testCannotUseInStaticAssert() {
  bool a{};
  static_assert(disclaim a);    // expected-error {{static assertion expression is not an integral constant expression}}
}

// TODO: make this work in constexpr
// constexpr bool testCanUseConstexprly(int val) {
//   MoveOnly a{42};
//   auto b = disclaim a;
//   return b.val_ % 2 == 0;
// }
// static_assert(testCanUseConstexprly(4));
// static_assert(!testCanUseConstexprly(5));

void testStructuredBindings() {
  struct S {
    MoveOnly x;
    MoveOnly y;
  } s{42, 64};
  auto [a, b] = disclaim s;
  disclaim a;                   // expected-error {{can only disclaim local variables}}
}

template<typename T>
void testTemplateDisclaim() {
  T x;
  auto y = disclaim x;
}
void instantiateTemplate() {
  testTemplateDisclaim<int>();
}

void testReference() {
  int a = 42;
  int& r = a;
  disclaim r;                  // expected-error {{cannot disclaim references}}
}

void testPointerDereference() {
  int a = 42;
  int* p = &a;
  disclaim *p;                 // expected-error {{can only disclaim local variables}}
}

struct S {
  int x;
  void testCannotDisclaimMembers() {
    disclaim x;               // expected-error {{can only disclaim local variables}}
  }
};

void testInitList() {
  MoveOnly a{42};
  MoveOnly arr[] = {disclaim a}; // should work
}

void testDecltype() {
  MoveOnly a{42};
  decltype(disclaim a) b = MoveOnly{7}; // expected-error {{'disclaim' keyword not allowed in unevaluated contexts}}
}

void testTryCatch() {
  try {
    MoveOnly a{42};
    throw disclaim a;
  } catch (MoveOnly& m) {
  }
}

void testDefaultArgument(int x = disclaim x) {}  // expected-error {{default argument references parameter 'x'}}

void testMultiDecl() {
  MoveOnly a{1}, b{2};
  // TODO: this should give a different warning than "has no effect" because it does have an effect, it disclaims a
  disclaim a, b;                // expected-warning {{left operand of comma operator has no effect}} expected-warning {{expression result unused}}
}

void testDecltypeAuto() {
  MoveOnly a{42};
  decltype(auto) b = disclaim a; // should deduce `MoveOnly`
}

#define DO_DISCLAIM(x) disclaim x
void testMacro() {
  MoveOnly a{42};
  DO_DISCLAIM(a);              // verify macro expansion is handled correctly
}
#undef DO_DISCLAIM

void testRangeFor() {
  int x[] {1, 2, 3};
  for (int a : x) {
    disclaim a;                // expected-error {{cannot disclaim a variable not defined in the same scope}}
  }
}

void testImmovable() {
  Immovable a{42};
  auto b = disclaim a;        // expected-error {{Immovable has a default-deleted operator disclaim because its move constructor is deleted}}
}

void testOnlyDisclaimable() {
  OnlyDisclaimable a{42};
  auto b = disclaim a;
}