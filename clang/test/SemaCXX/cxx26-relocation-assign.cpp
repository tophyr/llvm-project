// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct Good {
  Good &operator=(Good reloc);
};

struct NamedGood {
  NamedGood &operator=(NamedGood src reloc);
};

struct BadType {
  BadType &operator=(int reloc); // expected-error {{'reloc' parameter must have the same unqualified type as its enclosing class}}
};

struct TemplateBad {
  template <class T>
  TemplateBad &operator=(TemplateBad reloc); // expected-error {{'reloc' parameter must have the same unqualified type as its enclosing class}}
};

struct BadContext {
  void assign(BadContext reloc); // expected-error {{'reloc' parameter syntax is only valid on a constructor or assignment operator parameter}}
};
