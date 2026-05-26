// RUN: %clang_cc1 -std=c++26 -frelocation %s -triple x86_64-unknown-linux-gnu -emit-llvm-only -fdump-vtable-layouts > %t
// RUN: FileCheck %s < %t

namespace IntroducesSlice {
struct Base {
  Base(Base reloc) = default;
  virtual ~Base() = default;
};

void anchor(Base *ptr) { delete ptr; }

// CHECK-LABEL: Vtable for 'IntroducesSlice::Base' (5 entries).
// CHECK-NEXT:  0 | offset_to_top (0)
// CHECK-NEXT:  1 | IntroducesSlice::Base RTTI
// CHECK-NEXT:      -- (IntroducesSlice::Base, 0) vtable address --
// CHECK-NEXT:  2 | IntroducesSlice::Base::~Base() [complete]
// CHECK-NEXT:  3 | IntroducesSlice::Base::~Base() [deleting]
// CHECK-NEXT:  4 | void IntroducesSlice::Base::__reloc_slice(const void *, void *, bool)
//
// CHECK-LABEL: VTable indices for 'IntroducesSlice::Base' (3 entries).
// CHECK-NEXT:  0 | IntroducesSlice::Base::~Base() [complete]
// CHECK-NEXT:  1 | IntroducesSlice::Base::~Base() [deleting]
// CHECK-NEXT:  2 | void IntroducesSlice::Base::__reloc_slice(const void *, void *, bool)
}

namespace InheritsSlice {
struct Base {
  Base(Base reloc) = default;
  virtual ~Base() = default;
};

struct Derived : Base {
  Derived(Derived reloc) = default;
};

void anchor(Derived *ptr) { delete ptr; }

// CHECK-LABEL: Vtable for 'InheritsSlice::Derived' (5 entries).
// CHECK-NEXT:  0 | offset_to_top (0)
// CHECK-NEXT:  1 | InheritsSlice::Derived RTTI
// CHECK-NEXT:      -- (InheritsSlice::Base, 0) vtable address --
// CHECK-NEXT:      -- (InheritsSlice::Derived, 0) vtable address --
// CHECK-NEXT:  2 | InheritsSlice::Derived::~Derived() [complete]
// CHECK-NEXT:  3 | InheritsSlice::Derived::~Derived() [deleting]
// CHECK-NEXT:  4 | void InheritsSlice::Derived::__reloc_slice(const void *, void *, bool)
//
// CHECK-LABEL: VTable indices for 'InheritsSlice::Derived' (3 entries).
// CHECK-NEXT:  0 | InheritsSlice::Derived::~Derived() [complete]
// CHECK-NEXT:  1 | InheritsSlice::Derived::~Derived() [deleting]
// CHECK-NEXT:  2 | void InheritsSlice::Derived::__reloc_slice(const void *, void *, bool)
}

namespace NoImplicitSlice {
struct Base {
  virtual ~Base() = default;
};

// CHECK-LABEL: Vtable for 'NoImplicitSlice::Derived' (4 entries).
// CHECK-NEXT:  0 | offset_to_top (0)
// CHECK-NEXT:  1 | NoImplicitSlice::Derived RTTI
// CHECK-NEXT:      -- (NoImplicitSlice::Base, 0) vtable address --
// CHECK-NEXT:      -- (NoImplicitSlice::Derived, 0) vtable address --
// CHECK-NEXT:  2 | NoImplicitSlice::Derived::~Derived() [complete]
// CHECK-NEXT:  3 | NoImplicitSlice::Derived::~Derived() [deleting]
struct Derived : Base {};

void anchor(Derived *ptr) { delete ptr; }
}
