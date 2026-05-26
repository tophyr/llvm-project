// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

namespace std {
class type_info;
template <class T> struct tuple_size;
template <unsigned I, class T> struct tuple_element;
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

struct VirtualBase {
  int v;
};

struct VirtualDerivedBase : virtual VirtualBase {
  int b;
};

struct VirtualDerived : VirtualDerivedBase {
  int c;
};

struct VirtualOwner : VirtualDerived {
  int d;
};

struct UserDtor {
  UserDtor();
  ~UserDtor();
};

struct Pair {
  S first;
  S second;
};

namespace get_all_ns {
struct Leaf {
  int value;
};

struct Pair {
  Leaf first;
  Leaf second;
};

struct Wrapper {
private:
  Leaf first;
  Leaf second;

  friend Pair get_all(Wrapper self) { return {self.first, self.second}; }
};

struct Box {
private:
  Wrapper inner;

  friend Wrapper get_all(Box self) { return self.inner; }
};
} // namespace get_all_ns

struct BasePair {
  B first;
  B second;
};

struct TupleLike {
  S first;
  S second;
};

namespace std {
template <> struct tuple_size<TupleLike> { static constexpr int value = 2; };
template <> struct tuple_element<0, TupleLike> { using type = S; };
template <> struct tuple_element<1, TupleLike> { using type = S; };
}

template <unsigned I>
S &get(TupleLike &);

template <>
S &get<0>(TupleLike &value) {
  return value.first;
}

template <>
S &get<1>(TupleLike &value) {
  return value.second;
}

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

void take_by_value(S);
void take_by_reloc(S reloc);

void good() {
  S local reloc;
  S init reloc = S{};
  int other = reloc local.value;
  (void)other;

  Pair pair = {};
  auto [first, second] reloc = pair;
  static_assert(__is_same(decltype(first), S));
  static_assert(__is_same(decltype(second), S));
  static_assert(__is_same(decltype((first)), S &));
  static_assert(__is_same(decltype((second)), S &));
  static_assert(__is_same(decltype(first.this), void *));
  static_assert(__is_same(decltype(second.this), void *));
  (void)first.this;
  (void)second.this;
  (void)first;
  (void)sizeof((first));
  (void)typeid((first));
  (void)reloc first; // expected-note {{object relocated here}}
  (void)second;
  (void)first; // expected-error {{object 'first' cannot be used after it has been relocated}}

  TupleLike tuple_like = {};
  auto [tuple_first, tuple_second] reloc = tuple_like;
  static_assert(__is_same(decltype(tuple_first), S));
  static_assert(__is_same(decltype(tuple_second), S));
  (void)reloc tuple_first; // expected-note {{object relocated here}}
  (void)tuple_second;
  (void)tuple_first; // expected-error {{object 'tuple_first' cannot be used after it has been relocated}}

  auto lambda = [lambda_first = S{}, lambda_second = S{}] {};
  auto [closure_first, closure_second] reloc = lambda;
  static_assert(__is_same(decltype(closure_first), S));
  static_assert(__is_same(decltype(closure_second), S));
  (void)reloc closure_first; // expected-note {{object relocated here}}
  (void)closure_second;
  (void)closure_first; // expected-error {{object 'closure_first' cannot be used after it has been relocated}}

  auto [captured_first, captured_second] reloc = pair;
  auto copy_capture = [captured_first] { return 0; };
  auto ref_capture = [&captured_second] { return 0; };
  (void)copy_capture();
  (void)ref_capture();

  get_all_ns::Box get_all_box = {};
  auto [get_all_first, get_all_second] reloc = get_all_box;
  static_assert(__is_same(decltype(get_all_first), get_all_ns::Leaf));
  static_assert(__is_same(decltype(get_all_second), get_all_ns::Leaf));
  (void)reloc get_all_first; // expected-note {{object relocated here}}
  (void)get_all_second;
  (void)get_all_first; // expected-error {{object 'get_all_first' cannot be used after it has been relocated}}

  take_by_value(captured_first);
  take_by_reloc(captured_second);

  BasePair base_pair = {};
  auto [base_first, base_second] reloc = base_pair;
  int B::*member_ptr = &B::b;
  (void)(base_first.*member_ptr);
  (void)(base_second.*member_ptr);
  (void)reloc base_first.A; // expected-note {{object relocated here}}
  (void)base_first.A; // expected-error {{subobject 'A' of decomposed object 'base_first' cannot be used after it has been relocated}}
  (void)base_first.a; // expected-error {{subobject 'A' of decomposed object 'base_first' cannot be used after it has been relocated}} expected-note@-2 {{object relocated here}}
  (void)base_first.b;
  (void)base_second.A;

  int arr[2] = {1, 2};
  auto [x, y] reloc = arr;
  static_assert(__is_same(decltype(x), int));
  static_assert(__is_same(decltype((x)), int &));
  (void)x;
  (void)y;
  (void)reloc x; // expected-note {{object relocated here}}
  (void)y;
  (void)x; // expected-error {{object 'x' cannot be used after it has been relocated}}
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

void virtual_base_expr_uses() {
  VirtualOwner local reloc;
  auto semi reloc = reloc local.VirtualDerived;
  static_assert(__is_same(decltype(semi), VirtualDerived));
  (void)semi.b;
  (void)semi.c;
  (void)reloc semi.VirtualBase; // expected-error {{'reloc' operand must name a complete object}}
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
  Pair pair = {};
  auto [plain_first, plain_second] = pair;
  (void)reloc plain_first; // expected-error {{'reloc' operand cannot name a structured binding}}
  auto &[ref_first, ref_second] reloc = pair; // expected-error {{'reloc' may only be applied to a non-union class object of non-reference type}}
}
