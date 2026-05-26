// RUN: %clang_cc1 -std=c++26 -frelocation %s -triple x86_64-pc-windows-msvc -emit-llvm-only -fdump-vtable-layouts > %t
// RUN: FileCheck %s < %t
// RUN: %clang_cc1 -std=c++26 -frelocation %s -triple x86_64-pc-windows-msvc -emit-llvm -o - | FileCheck %s --check-prefix=IR

struct Base {
  Base(Base reloc) = default;
  virtual ~Base() = default;
};

struct Derived : Base {
  int value;
  Derived(Derived reloc) = default;
};

Base use(Base *ptr) { return __builtin_reloc_and_uninitialize(ptr); }
void anchor(Derived *ptr) { delete ptr; }
void consume(Derived *ptr) { (void)use(ptr); }

// CHECK-LABEL: VFTable for 'Base' (3 entries).
// CHECK-NEXT:    0 | Base RTTI
// CHECK-NEXT:    1 | Base::~Base() [scalar deleting]
// CHECK-NEXT:    2 | void Base::__reloc_slice(const void *, void *, bool)
//
// CHECK-LABEL: VFTable for 'Base' in 'Derived' (3 entries).
// CHECK-NEXT:    0 | Derived RTTI
// CHECK-NEXT:    1 | Derived::~Derived() [scalar deleting]
// CHECK-NEXT:    2 | void Derived::__reloc_slice(const void *, void *, bool)
//
// CHECK-LABEL: VFTable indices for 'Derived' (2 entries).
// CHECK-NEXT:    0 | Derived::~Derived() [scalar deleting]
// CHECK-NEXT:    1 | void Derived::__reloc_slice(const void *, void *, bool)

// IR-LABEL: define dso_local void @"?use@@YA?AUBase@@PEAU1@@Z"
// IR: %vfn = getelementptr inbounds ptr, ptr %vtable, i64 1
// IR: call void %{{.*}}(ptr noundef nonnull align 8 dereferenceable(8) %{{.*}}, ptr noundef @"??_R0?AUBase@@@8", ptr noundef %agg.result, i1 noundef zeroext false)
