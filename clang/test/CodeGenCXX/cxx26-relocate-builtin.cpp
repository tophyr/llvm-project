// RUN: %clang_cc1 -std=c++26 -frelocation -triple x86_64-unknown-linux-gnu -emit-llvm -disable-llvm-passes %s -o - | FileCheck %s

struct PlainDelete {
  static int deletes;
  PlainDelete();
  PlainDelete(PlainDelete reloc);
  ~PlainDelete();
  static void operator delete(void *);
};

PlainDelete __builtin_reloc_and_reclaim_use(PlainDelete *p) {
  // CHECK-LABEL: define{{.*}} @_Z31__builtin_reloc_and_reclaim_useP11PlainDelete
  // CHECK: call void @_ZN11PlainDeleteC1ES_
  // CHECK: call void @_ZN11PlainDeletedlEPv
  return __builtin_reloc_and_reclaim(p);
}

struct Base {
  Base();
  Base(Base reloc);
  virtual ~Base() = default;
  virtual void key();
};

struct Derived : Base {
  Derived();
  Derived(Derived reloc);
  ~Derived() override = default;
  void key() override;
};

Base __builtin_reloc_and_uninitialize_virtual(Base *p) {
  // CHECK-LABEL: define{{.*}} @_Z40__builtin_reloc_and_uninitialize_virtualP4Base
  // CHECK: %vfn = getelementptr inbounds ptr, ptr %vtable, i64 3
  // CHECK: call void %{{.*}}(ptr noundef nonnull align 8 dereferenceable(8) %{{.*}}, ptr noundef @_ZTI4Base, ptr noundef %agg.result, i1 noundef zeroext false)
  return __builtin_reloc_and_uninitialize(p);
}
