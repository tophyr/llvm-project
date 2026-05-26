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

struct ImplicitGood {};
using ImplicitGoodAssign = ImplicitGood &(ImplicitGood::*)(ImplicitGood);
ImplicitGoodAssign implicit_good_assign = &ImplicitGood::operator=;

struct CrashBase {
  int a;
};

struct CrashMember : CrashBase {
  int b;
};

struct CrashWrap {
  CrashMember first;
  CrashMember second;
};

using CrashWrapAssign = CrashWrap &(CrashWrap::*)(CrashWrap);
CrashWrapAssign crash_wrap_assign = &CrashWrap::operator=;

struct DefaultedMember {
  DefaultedMember &operator=(DefaultedMember &&);
};

struct DefaultedBase {
  DefaultedBase &operator=(DefaultedBase &&);
};

struct DefaultedGood : DefaultedBase {
  DefaultedMember member;
  DefaultedGood &operator=(DefaultedGood src reloc) = default;
};

struct TrivialCopyAssignFallbackMember {
  int value;
  TrivialCopyAssignFallbackMember &
  operator=(const TrivialCopyAssignFallbackMember &) = default;
  TrivialCopyAssignFallbackMember &
  operator=(TrivialCopyAssignFallbackMember &&) = delete;
};

struct TrivialDefaultedCopyAssignFallback {
  TrivialCopyAssignFallbackMember member;
  TrivialDefaultedCopyAssignFallback &
  operator=(TrivialDefaultedCopyAssignFallback src reloc) = default;
};


struct DefaultedBadConst {
  DefaultedBadConst &operator=(const DefaultedBadConst src reloc) = default; // expected-warning {{explicitly defaulted move assignment operator is implicitly deleted}}
  // expected-note@-1 {{function is implicitly deleted because its declared type does not match the type of an implicit move assignment operator}}
};

struct DefaultedBad {
  DefaultedBad &operator=(DefaultedBad reloc) = default; // expected-error {{only decomposing relocation assignment operators may be defaulted}}
};

struct OutOfLineDefaultedBad {
  OutOfLineDefaultedBad &operator=(OutOfLineDefaultedBad reloc);
};

inline OutOfLineDefaultedBad &
OutOfLineDefaultedBad::operator=(OutOfLineDefaultedBad reloc) = default; // expected-error {{only decomposing relocation assignment operators may be defaulted}}

struct OutOfLineDefaultedBadConst {
  OutOfLineDefaultedBadConst &
  operator=(const OutOfLineDefaultedBadConst src reloc);
};

inline OutOfLineDefaultedBadConst &
OutOfLineDefaultedBadConst::operator=( // expected-error {{an explicitly-defaulted decomposing relocation assignment operator must have parameter type 'OutOfLineDefaultedBadConst'}}
    const OutOfLineDefaultedBadConst src reloc) = default;
