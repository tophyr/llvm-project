// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct S {
  int value;
};

struct U {
  int value;
};

union UnionS {
  S s;
};

void good() {
  S local reloc;
  S init reloc = S{};
  S other = reloc local;
  (void)other;
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
}
