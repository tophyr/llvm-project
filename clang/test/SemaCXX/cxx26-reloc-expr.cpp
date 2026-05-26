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
struct Base {};
ByLRef base_object(Base &);
ByRRef base_object(Base &&);
ByValue base_object(Base);
struct Pair {
  S first;
  S second;
};
struct Derived : Base {
  S member;
};
struct IndirectBase : Base {
  IndirectBase();
  IndirectBase(const IndirectBase &);
  IndirectBase &operator=(const IndirectBase &);
};
struct TwiceIndirect : IndirectBase {
  TwiceIndirect();
  TwiceIndirect(const TwiceIndirect &);
  TwiceIndirect &operator=(const TwiceIndirect &);
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

struct Holder {
  Pair pair;
  Derived derived;

  Holder(Holder src reloc) {
    Pair pair reloc = Pair{};
    static_assert(__is_same(decltype(object(reloc src.pair.first)), ByValue));
    static_assert(__is_same(decltype(object(reloc pair.second)), ByValue));
    static_assert(__is_same(
        decltype(base_object(reloc static_cast<Base &>(src.derived))), ByValue));
  }
};

struct RelocDirectBase : Base {
  RelocDirectBase(RelocDirectBase src reloc) {
    (void)base_object(reloc src.Base);
  }
};

template <class T>
auto forward_scalar(T &&t) -> decltype(forward_only(reloc t));

int global_int = 0;
static_assert(__is_same(decltype(forward_scalar(global_int)), ByForwardLRef));
static_assert(__is_same(decltype(forward_scalar(0)), ByForwardRRef));

int global = 0;
void func();

void ambiguous_without_reloc(int p, S s) {
  (void)scalar(p); // expected-error {{call to 'scalar' is ambiguous}}
                   // expected-note@9 {{candidate function}}
                   // expected-note@11 {{candidate function}}
  (void)object(s); // expected-error {{call to 'object' is ambiguous}}
                   // expected-note@16 {{candidate function}}
                   // expected-note@18 {{candidate function}}
}

void bad_cases() {
  Pair pair;
  Derived derived;
  TwiceIndirect indirect reloc;
  (void)object(reloc pair.first); // expected-error {{'reloc' operand must name a complete object}}
  (void)base_object(reloc static_cast<Base &>(derived)); // expected-error {{'reloc' operand must name a complete object}}
  (void)base_object(reloc indirect.Base); // expected-error {{'reloc' operand must name a complete object}}
  auto [binding_first, binding_second] = pair;
  (void)object(reloc binding_first); // expected-error {{'reloc' operand cannot name a structured binding}}
  auto [decomp_first, decomp_second] reloc = pair;
  (void)object(reloc decomp_first);
  (void)object(reloc decomp_second);
  [captured = pair.first] {
    (void)object(reloc captured); // expected-error {{'reloc' operand cannot name a lambda capture}}
  }();
  (void)scalar(reloc global); // expected-error {{'reloc' operand must name a local variable or parameter}}
  (void)(reloc func); // expected-error {{'reloc' operand must be a glvalue or prvalue of object type}}
}

void use_after_reloc_local() {
  S local;
  (void)object(reloc local);
  (void)local; // expected-error {{object 'local' cannot be used after it has been relocated}}
               // expected-note@-2 {{object relocated here}}
}

void use_after_reloc_local_member() {
  Derived local;
  Derived other = reloc local;
  (void)other;
  (void)local.member; // expected-error {{object 'local' cannot be used after it has been relocated}}
                      // expected-note@-3 {{object relocated here}}
}

void use_after_reloc_param(S param) {
  (void)object(reloc param);
  (void)param; // expected-error {{object 'param' cannot be used after it has been relocated}}
               // expected-note@-2 {{object relocated here}}
}

void repeated_reloc(S param) {
  (void)object(reloc param);
  (void)object(reloc param); // expected-error {{object 'param' cannot be used after it has been relocated}}
                             // expected-note@-2 {{object relocated here}}
}

void use_after_reloc_short_circuit_or(bool cond) {
  S local;
  if (!cond || ((void)object(reloc local), true)) {
    if (!cond)
      return;
    (void)local; // expected-error {{object 'local' cannot be used after it has been relocated}}
                 // expected-note@-4 {{object relocated here}}
  }
}

void use_after_reloc_switch(int n) {
  S local;
  switch (n) {
  case 0:
    (void)object(reloc local);
    break;
  case 1:
    break;
  }
  (void)local; // expected-error {{object 'local' cannot be used after it has been relocated}}
               // expected-note@-6 {{object relocated here}}
}

void use_after_reloc_loop_break() {
  S local;
  while (true) {
    (void)object(reloc local);
    break;
  }
  (void)local; // expected-error {{object 'local' cannot be used after it has been relocated}}
               // expected-note@-4 {{object relocated here}}
}

template <class T>
using id_t = T;

void unevaluated_after_reloc(S param) {
  (void)object(reloc param);
  using Param = id_t<decltype(param)>;
  (void)sizeof(Param);
}

struct DecomposedUseAfterReloc {
  Pair pair;

  DecomposedUseAfterReloc(DecomposedUseAfterReloc src reloc) {
    (void)object(reloc src.pair.first);
    (void)object(reloc src.pair.second);
  }
};
