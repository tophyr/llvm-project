// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

namespace std {
class type_info;
}

struct S {
  int value;

  void member();
  static void static_member();
};
// expected-note@7 {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'S' to 'const S &' for 1st argument}}
// expected-note@7 {{candidate constructor (the implicit move constructor) not viable: no known conversion from 'S' to 'S &&' for 1st argument}}
// expected-note@7 {{candidate constructor (the implicit default constructor) not viable: requires 0 arguments, but 1 was provided}}
// expected-note@7 {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'S' to 'const S &' for 1st argument}}
// expected-note@7 {{candidate constructor (the implicit move constructor) not viable: no known conversion from 'S' to 'S &&' for 1st argument}}
// expected-note@7 {{candidate constructor (the implicit default constructor) not viable: requires 0 arguments, but 1 was provided}}

struct Immobile {
  Immobile();
  Immobile(Immobile &&) = delete;
  Immobile(const Immobile &) = delete;
};

struct UserDtor {
  UserDtor();
  ~UserDtor();
};

struct RelocOnly {
  RelocOnly();
  RelocOnly(RelocOnly &&) = delete;
  RelocOnly(const RelocOnly &) = delete;
  RelocOnly(RelocOnly reloc);
};

struct FriendUserDtorParam {
  FriendUserDtorParam();
  ~FriendUserDtorParam();

  friend void friend_user_dtor_param(FriendUserDtorParam src reloc);
};

void friend_user_dtor_param(FriendUserDtorParam src reloc) {
  (void)src.this;
}

struct PrivilegedUserDtor {
  PrivilegedUserDtor();
  ~PrivilegedUserDtor();

  static void ok(PrivilegedUserDtor arg reloc) {
    (void)arg.this;
  }
};

void free_ok(S arg reloc) {
  (void)arg.value;
  static_assert(__is_same(decltype(arg), S));
  static_assert(__is_same(decltype(arg.this), void *));
  (void)arg.this;
  arg.static_member();
  arg.member(); // expected-error {{non-static member 'member' cannot be used through decomposed object 'arg'}}
  (void)sizeof(arg);
  (void)typeid(arg);
  (void)sizeof((arg)); // expected-error {{decomposed object 'arg' cannot be used as a value}}
  using BadDecltype = decltype((arg)); // expected-error {{decomposed object 'arg' cannot be parenthesized in a decltype operand}}
  (void)typeid((arg)); // expected-error {{decomposed object 'arg' cannot be used as a value}}
}

void redecl_mismatch_1(S arg reloc);
void redecl_mismatch_1(S arg); // expected-error {{'reloc' parameter mismatch in redeclaration of 'redecl_mismatch_1'}} expected-note@72 {{previous declaration is here}}

void redecl_mismatch_2(S arg);
void redecl_mismatch_2(S arg reloc); // expected-error {{'reloc' parameter mismatch in redeclaration of 'redecl_mismatch_2'}} expected-note@75 {{previous declaration is here}}

struct X {
  void good(S arg reloc);
  void bad(S arg); // expected-note {{passing argument to parameter 'arg' here}}
  void ptr_ok(S arg reloc) { (void)arg.value; }
};

void (X::*member_function_pointer_bad)(S reloc); // expected-error {{'reloc' parameter is only permitted in a function declaration}}
using MemberFunctionPointerBad = void (X::*)(S reloc); // expected-error {{'reloc' parameter is only permitted in a function declaration}}

void free_ptr_ok(S arg reloc) { (void)arg.value; }
void free_plain(S arg) { (void)arg.value; } // expected-note {{candidate function not viable: no known conversion from 'S' to 'S' for 1st argument}} expected-note {{passing argument to parameter 'arg' here}}
template <class T>
void free_template_ok(T arg reloc) {
  (void)arg.value;
}
void invoke_through_pointer() {
  void (*fp)(S) = free_ptr_ok;
  fp(S{});
}

void invoke_through_member_pointer(X &x) {
  void (X::*pmf)(S) = &X::ptr_ok;
  (x.*pmf)(S{});
}

void X::good(S arg reloc) {}
void X::bad(S arg reloc) {} // expected-error {{'reloc' parameter mismatch in redeclaration of 'bad'}} expected-note@80 {{previous declaration is here}}

static_assert(__is_same(decltype(&X::good), void (X::*)(S)));

void immobile_param(Immobile reloc); // expected-error {{'reloc' parameter type 'Immobile' must have an eligible relocation, move, or copy constructor}}
void reloc_only_param(RelocOnly reloc);
void user_dtor_param(UserDtor reloc); // expected-error {{'reloc' may not decompose type 'UserDtor' because its destructor is user-provided}}

void forward_decomposed_direct_free(S src reloc) {
  free_ok(src);
}

void forward_decomposed_direct_member(S src reloc, X &x) {
  x.good(src);
}

void explicit_forward_decomposed_direct(S src reloc) {
  free_ok(reloc src);
}

void forward_decomposed_template_free(S src reloc) {
  free_template_ok(src);
}

void forward_decomposed_indirect_free(S src reloc) {
  void (*fp)(S) = free_ptr_ok;
  fp(src);
}

void forward_decomposed_indirect_member(S src reloc, X &x) {
  void (X::*pmf)(S) = &X::ptr_ok;
  (x.*pmf)(src);
}

void reject_non_reloc_sinks(S src reloc, X &x) {
  free_plain(src); // expected-error {{no matching function for call to 'free_plain'}}
  x.bad(src); // expected-error {{no matching constructor for initialization of 'S'}}

  void (*fp)(S) = free_plain;
  fp(src); // expected-error {{no matching constructor for initialization of 'S'}}
}
