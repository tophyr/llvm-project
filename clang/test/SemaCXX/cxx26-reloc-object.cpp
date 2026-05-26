// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

namespace std {
class type_info;
}

struct S {
  int value;
};

struct U {
  int value;
  static int static_value;

  void member();
  static void static_member();
};

struct A {
  int a;

  A();
  A(const A &);
  A &operator=(const A &);
};

struct B : A {
  int b;

  B();
  B(const B &);
  B &operator=(const B &);
};

struct W : B {
  int w;

  W();
  W(const W &);
  W &operator=(const W &);
};

struct UserDtor {
  UserDtor();
  ~UserDtor();
};

struct PrivilegedUserDtor {
  PrivilegedUserDtor();
  ~PrivilegedUserDtor();

  static void ok() {
    PrivilegedUserDtor local reloc;
    (void)local.this;
  }
};

struct FriendUserDtor {
  FriendUserDtor();
  ~FriendUserDtor();

  friend void friend_local_ok();
};

void friend_local_ok() {
  FriendUserDtor local reloc;
  (void)local.this;
}

union UnionS {
  S s;
};

void good() {
  S local reloc;
  S init reloc = S{};
  int other = reloc local.value;
  (void)other;
}

void expr_uses() {
  U local reloc;
  (void)local; // expected-error {{decomposed object 'local' cannot be used as a value}}
  (void)local.value;
  (void)local.static_value;
  static_assert(__is_same(decltype(local.this), void *));
  (void)local.this;
  local.static_member();
  local.member(); // expected-error {{non-static member 'member' cannot be used through decomposed object 'local'}}
  (void)sizeof(local);
  static_assert(__is_same(decltype(local), U));
  (void)typeid(local);
  (void)sizeof((local)); // expected-error {{decomposed object 'local' cannot be used as a value}}
  using BadDecltype = decltype((local)); // expected-error {{decomposed object 'local' cannot be parenthesized in a decltype operand}}
  (void)typeid((local)); // expected-error {{decomposed object 'local' cannot be used as a value}}
}

void base_expr_uses() {
  W local reloc;
  (void)local.b;
  (void)local.a;
  static_assert(__is_same(decltype(local.B), B));
  static_assert(__is_same(decltype((local.B)), B &));
  static_assert(__is_same(decltype(local.B.this), void *));
  (void)local.B.b;
  (void)local.B.this;
  (void)typeid(local.B);
  (void)typeid((local.B));
  static_assert(__is_same(decltype(local.A), A));
  static_assert(__is_same(decltype((local.A)), A &));
  (void)local.A.a;
}

void const_expr_uses() {
  const U local reloc = {};
  static_assert(__is_same(decltype(local.this), const void *));
  (void)local.this;
  static_assert(__is_same(decltype(local.static_value), int));
}

void const_base_expr_uses() {
  const W local reloc = {};
  static_assert(__is_same(decltype(local.B), B));
  static_assert(__is_same(decltype((local.B)), const B &));
  static_assert(__is_same(decltype(local.A), A));
  static_assert(__is_same(decltype((local.A)), const A &));
}

void member_pointer_uses() {
  W local reloc;
  constexpr int W::*member_ptr = &W::w;
  int W::*dynamic_ptr = member_ptr;
  (void)(local.*member_ptr);
  (void)(local.*dynamic_ptr); // expected-error {{pointer-to-member access through decomposed object 'local' requires a constant-evaluated member pointer}}
}

S global reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}
static S global_static reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}
extern S global_extern reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}

struct MemberCases {
  S field reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}
};

void bad() {
  static S local_static reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}
  extern S local_extern reloc; // expected-error {{'reloc' may only appear on a function parameter or local variable definition}}
  int scalar reloc; // expected-error {{'reloc' may only be applied to a non-union class object of non-reference type}}
  int &ref = scalar;
  int &bad_ref reloc = ref; // expected-error {{'reloc' may only be applied to a non-union class object of non-reference type}}
  UnionS us reloc; // expected-error {{'reloc' may only be applied to a non-union class object of non-reference type}}
  UserDtor user reloc; // expected-error {{'reloc' may not decompose type 'UserDtor' because its destructor is user-provided}}
}
