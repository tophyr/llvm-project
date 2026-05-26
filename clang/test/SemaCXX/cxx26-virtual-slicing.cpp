// RUN: %clang_cc1 -std=c++26 -frelocation -fsyntax-only -verify %s

namespace UserProvidedDestructor {
struct Base { // expected-error {{virtual slicing function for 'Base' is ill-formed because its destructor is user-provided}}
  Base(Base reloc) = default;
  virtual ~Base() {} // expected-note {{member '~Base' declared here}}
};
}

namespace NoEligibleCtor {
struct Base {
  Base(Base reloc) = default;
  virtual ~Base() = default;
};

struct Immobile {
  Immobile() = default;
  Immobile(const Immobile &) = delete;
  Immobile(Immobile &&) = delete;
};

struct Derived : Base { // expected-error {{virtual slicing function for 'Derived' is ill-formed because it has no eligible relocation, move, or copy constructor}}
  Immobile member;
  virtual ~Derived() = default;
};
}
