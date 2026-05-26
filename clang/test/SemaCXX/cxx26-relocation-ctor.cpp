// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct Good {
  Good(Good reloc);
};

struct NamedGood {
  NamedGood(NamedGood src reloc);
};

struct WithDefaultArg {
  WithDefaultArg(WithDefaultArg reloc, int = 0);
};

struct BadType {
  BadType(int reloc); // expected-error {{'reloc' parameter must have the same unqualified type as its enclosing class}}
};

struct BadOrder {
  BadOrder(int, BadOrder reloc); // expected-error {{'reloc' parameter must be the first parameter of a constructor or assignment operator}}
};

struct TemplateBad {
  template <class T>
  TemplateBad(TemplateBad reloc, T); // expected-error {{'reloc' parameter must have the same unqualified type as its enclosing class}}
};

void not_a_ctor(int reloc); // expected-error {{'reloc' parameter syntax is only valid on a constructor or assignment operator parameter}}
void also_not_a_ctor(int x reloc); // expected-error {{'reloc' parameter syntax is only valid on a constructor or assignment operator parameter}}

struct NamedRelocStillAllowedOutsideParams {
  int reloc;
};
