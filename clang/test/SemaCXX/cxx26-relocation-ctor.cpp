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

struct DelegatingReloc {
  DelegatingReloc(int);
  DelegatingReloc(DelegatingReloc src reloc) : DelegatingReloc(0) {} // expected-error {{delegating relocation constructors are not supported}}
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

struct RelocOnlyMember {
  RelocOnlyMember() = delete;
  RelocOnlyMember(int);
  RelocOnlyMember(const RelocOnlyMember &) = delete;
  RelocOnlyMember(RelocOnlyMember &&) = delete;
  RelocOnlyMember(RelocOnlyMember reloc) = default;
};

struct ImplicitGood {
  RelocOnlyMember member;
  ImplicitGood(int) : member(0) {}
};

void test_implicit_relocation_ctor() {
  ImplicitGood src(0);
  ImplicitGood dst = reloc src;
  (void)dst;
}

struct DefaultedGoodMember {
  DefaultedGoodMember();
  DefaultedGoodMember(DefaultedGoodMember &&);
};

struct DefaultedGood {
  DefaultedGoodMember member;
  DefaultedGood(DefaultedGood reloc) = default;
};

struct TrivialCopyFallbackMember {
  int value;
  TrivialCopyFallbackMember() = default;
  TrivialCopyFallbackMember(const TrivialCopyFallbackMember &) = default;
  TrivialCopyFallbackMember(TrivialCopyFallbackMember &&) = delete;
};

struct TrivialDefaultedCopyFallback {
  TrivialCopyFallbackMember member;
  TrivialDefaultedCopyFallback(TrivialDefaultedCopyFallback reloc) = default;
};


struct DefaultedBadConst {
  DefaultedBadConst(const DefaultedBadConst reloc) = default; // expected-warning {{explicitly defaulted move constructor is implicitly deleted}}
  // expected-note@-1 {{function is implicitly deleted because its declared type does not match the type of an implicit move constructor}}
};

struct OutOfLineBadConst {
  OutOfLineBadConst(const OutOfLineBadConst reloc);
};

inline OutOfLineBadConst::OutOfLineBadConst( // expected-error {{an explicitly-defaulted relocation constructor must have parameter type 'OutOfLineBadConst'}}
    const OutOfLineBadConst reloc) = default;

struct ImplicitRelocNoteMember {
  ImplicitRelocNoteMember() = delete;
  ImplicitRelocNoteMember(int);
  ImplicitRelocNoteMember(const ImplicitRelocNoteMember &) = delete;
  ImplicitRelocNoteMember(ImplicitRelocNoteMember &&) = delete;
  ImplicitRelocNoteMember(ImplicitRelocNoteMember reloc) = default;
};

struct ImplicitRelocNote { // expected-note {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'const char[2]' to 'const ImplicitRelocNote &' for 1st argument}}
  ImplicitRelocNoteMember member;
  ImplicitRelocNote(int) : member(0) {} // expected-note {{candidate constructor not viable: no known conversion from 'const char[2]' to 'int' for 1st argument}}
};

void test_implicit_reloc_ctor_note() {
  ImplicitRelocNote bad = "x"; // expected-error {{no viable conversion from 'const char[2]' to 'ImplicitRelocNote'}}
  (void)bad;
}
