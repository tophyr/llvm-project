// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct ByLRef {};
struct ByRRef {};
struct ByForwardLRef {};
struct ByForwardRRef {};
struct ByValue {};

ByLRef scalar(int &);
ByRRef scalar(int &&);
ByValue scalar(int);
ByForwardLRef forward_only(int &);
ByForwardRRef forward_only(int &&);

struct S {};
ByLRef object(S &);
ByRRef object(S &&);
ByValue object(S);
struct Pair {
  S first;
  S second;
};

void local_cases(int p, S s) {
  int local = 0;
  int &lref = local;
  int &&rref = 0;

  static_assert(__is_same(decltype(scalar(reloc local)), ByValue));
  static_assert(__is_same(decltype(scalar(reloc p)), ByValue));
  static_assert(__is_same(decltype(scalar(reloc 0)), ByValue));
  static_assert(__is_same(decltype(forward_only(reloc lref)), ByForwardLRef));
  static_assert(__is_same(decltype(forward_only(reloc rref)), ByForwardRRef));

  static_assert(__is_same(decltype(object(reloc s)), ByValue));
  static_assert(__is_same(decltype(object(reloc S{})), ByValue));
}

template <class T>
auto forward_scalar(T &&t) -> decltype(forward_only(reloc t));

int global_int = 0;
static_assert(__is_same(decltype(forward_scalar(global_int)), ByForwardLRef));
static_assert(__is_same(decltype(forward_scalar(0)), ByForwardRRef));

int global = 0;
void func();

void ambiguous_without_reloc(int p, S s) {
  (void)scalar(p); // expected-error {{call to 'scalar' is ambiguous}}
                   // expected-note@-42 {{candidate function}}
                   // expected-note@-41 {{candidate function}}
  (void)object(s); // expected-error {{call to 'object' is ambiguous}}
                   // expected-note@-38 {{candidate function}}
                   // expected-note@-37 {{candidate function}}
}

void bad_cases() {
  Pair pair;
  (void)object(reloc pair.first); // expected-error {{'reloc' operand must name a complete object}}
  auto [binding_first, binding_second] = pair;
  (void)object(reloc binding_first); // expected-error {{'reloc' operand cannot name a structured binding}}
  [captured = pair.first] {
    (void)object(reloc captured); // expected-error {{'reloc' operand cannot name a lambda capture}}
  }();
  (void)scalar(reloc global); // expected-error {{'reloc' operand must name a local variable or parameter}}
  (void)(reloc func); // expected-error {{'reloc' operand must be a glvalue or prvalue of object type}}
}
