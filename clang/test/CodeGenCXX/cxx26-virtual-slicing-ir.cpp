// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -emit-llvm -disable-llvm-passes %s -o - | FileCheck %s

namespace SameTypeReloc {
struct Base {
  Base() = default;
  Base(Base reloc);
  virtual ~Base() = default;
  virtual void key();
};

Base::Base(Base src reloc) {}
void Base::key() {}

// CHECK-LABEL: define linkonce_odr void @_ZN13SameTypeReloc4Base13__reloc_sliceEPKvPvb(
// CHECK: %[[MATCH:[0-9]+]] = icmp eq ptr %{{[^,]+}}, @_ZTIN13SameTypeReloc4BaseE
// CHECK: br i1 %[[MATCH]], label %slice.same, label %slice.dispatch
// CHECK: slice.same:
// CHECK: call void @_ZN13SameTypeReloc4BaseC1E{{S_|S0_}}(ptr %{{[^,]+}}, ptr %{{[^)]+}})
// CHECK: slice.return:
// CHECK: br i1 %[[BOOL:[^,]+]], label %slice.delete, label %slice.delete.cont
// CHECK: slice.delete:
// CHECK: call void @_ZdlPvm(ptr noundef %{{[^,]+}}, i64 noundef 8)
}

namespace BaseForwarding {
struct Counted {
  int *p;
  Counted(int *p) : p(p) {}
  Counted(Counted reloc) = default;
  ~Counted() { ++*p; }
};

struct Base {
  Base() = default;
  Base(Base reloc) = default;
  virtual ~Base() = default;
  virtual void key();
};

void Base::key() {}

struct Derived : Base {
  Counted c;
  Derived(int *p) : Base(), c(p) {}
  Derived(Derived reloc) = default;
  ~Derived() = default;
  void key() override;
};

void Derived::key() {}

// CHECK-LABEL: define linkonce_odr void @_ZN14BaseForwarding7Derived13__reloc_sliceEPKvPvb(
// CHECK: %[[SELF:[0-9]+]] = icmp eq ptr %{{[^,]+}}, @_ZTIN14BaseForwarding7DerivedE
// CHECK: br i1 %[[SELF]], label %slice.same, label %slice.dispatch
// CHECK: slice.dispatch:
// CHECK: %[[BASE:[0-9]+]] = icmp eq ptr %{{[^,]+}}, @_ZTIN14BaseForwarding4BaseE
// CHECK: br i1 %[[BASE]], label %slice.base, label %slice.return
// CHECK: slice.base:
// CHECK: call void @_ZN14BaseForwarding4Base13__reloc_sliceEPKvPvb(ptr noundef nonnull align 8 dereferenceable(8) %{{[^,]+}}, ptr noundef %{{[^,]+}}, ptr noundef %{{[^,]+}}, i1 noundef zeroext false)
// CHECK: call void @_ZN14BaseForwarding7CountedD1Ev(ptr noundef nonnull align 8 dereferenceable(8) %{{[^)]+}}) #[[DTOR_ATTR:[0-9]+]]
}

namespace MoveFallback {
struct Counted {
  int *p;
  Counted(int *p) : p(p) {}
  Counted(const Counted &) = default;
  Counted(Counted &&) = default;
  ~Counted() { ++*p; }
};

struct Base {
  Base() = default;
  Base(Base reloc) = default;
  virtual ~Base() = default;
  virtual void key();
};

void Base::key() {}

struct Derived : Base {
  Counted c;
  Derived(int *p) : Base(), c(p) {}
  Derived(Derived &&) = default;
  ~Derived() = default;
  void key() override;
};

void Derived::key() {}

// CHECK-LABEL: define linkonce_odr void @_ZN12MoveFallback7Derived13__reloc_sliceEPKvPvb(
// CHECK: slice.same:
// CHECK: call void @_ZN12MoveFallback7DerivedC1EO{{S_|S0_}}(ptr %{{[^,]+}}, ptr %{{[^)]+}})
// CHECK-NEXT: call void @_ZN12MoveFallback7DerivedD1Ev(ptr noundef nonnull align 8 dereferenceable(16) %{{[^)]+}}) #[[DTOR_ATTR]]
}
