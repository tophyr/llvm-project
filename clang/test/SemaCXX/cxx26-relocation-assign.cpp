// RUN: %clang_cc1 -std=c++26 -frelocation -fsyntax-only -verify %s

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
  void assign(BadContext reloc);
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

struct CopyAssignFallbackMember {
  CopyAssignFallbackMember &operator=(const CopyAssignFallbackMember &);
  CopyAssignFallbackMember &operator=(CopyAssignFallbackMember &&) = delete;
};

struct DefaultedCopyFallback {
  CopyAssignFallbackMember member;
  DefaultedCopyFallback &operator=(DefaultedCopyFallback src reloc) = default;
};

struct ImplicitCopyFallback {
  CopyAssignFallbackMember member;
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

using ImplicitCopyFallbackAssign =
    ImplicitCopyFallback &(ImplicitCopyFallback::*)(ImplicitCopyFallback);
ImplicitCopyFallbackAssign implicit_copy_fallback_assign =
    &ImplicitCopyFallback::operator=;

void test_implicit_copy_fallback_assign(ImplicitCopyFallback a,
                                        ImplicitCopyFallback b,
                                        ImplicitCopyFallback c) {
  a = reloc b;
  a.operator=(reloc c);
}

struct DeletedImplicitRelocAssign {
  int &ref;
};

void test_deleted_implicit_reloc_assign_note(DeletedImplicitRelocAssign a,
                                             DeletedImplicitRelocAssign b) {
  a.operator=(reloc b); // expected-error {{call to deleted member function 'operator='}}
  // expected-note@-7 {{candidate function (the implicit copy assignment operator) has been implicitly deleted}}
  // expected-note@-8 {{candidate function (the implicit relocation assignment operator) has been implicitly deleted}}
}

struct ConstexprAssign {
  int value;
  constexpr ConstexprAssign(int value = 0) : value(value) {}
  constexpr ConstexprAssign &operator=(ConstexprAssign src reloc) {
    value = src.value;
    return *this;
  }
};

constexpr bool test_constexpr_reloc_assign() {
  ConstexprAssign dst(0);
  ConstexprAssign src(1);
  dst = reloc src;
  return dst.value == 1;
}

static_assert(test_constexpr_reloc_assign());

struct ConstexprDefaultedAssign {
  int value;
  constexpr ConstexprDefaultedAssign(int value = 0) : value(value) {}
  constexpr ConstexprDefaultedAssign &
  operator=(ConstexprDefaultedAssign src reloc) = default;
};

constexpr bool test_constexpr_defaulted_reloc_assign() {
  ConstexprDefaultedAssign dst(0);
  ConstexprDefaultedAssign src(1);
  dst = reloc src;
  return dst.value == 1;
}

static_assert(test_constexpr_defaulted_reloc_assign());

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
