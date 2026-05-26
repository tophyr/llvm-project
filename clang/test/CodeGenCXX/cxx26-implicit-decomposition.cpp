// RUN: %clang_cc1 -std=c++26 -frelocation -emit-llvm -O0 -disable-llvm-passes %s -o - | FileCheck %s

struct Tracker {
  int *p;
  Tracker(int *p) : p(p) {}
  Tracker(const Tracker &) = delete;
  Tracker(Tracker &&) = delete;
  Tracker(Tracker reloc) = default;
  ~Tracker() { ++*p; }
};

struct Pair {
  Tracker first;
  Tracker second;
};

struct ArrayBox {
  Tracker elements[2];
};

constexpr Tracker Pair::*PairFirst = &Pair::first;

// CHECK-LABEL: define dso_local i32 @f(
extern "C" int f(int *a, int *b) {
  Tracker t = Pair{Tracker(a), Tracker(b)}.first;
  return *a + 10 * *b;
}
// CHECK: %[[FIRST:[^ ]+]] = getelementptr inbounds nuw %struct.Pair, ptr %ref.tmp, i32 0, i32 0
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[FIRST]], ptr noundef
// CHECK: %[[SECOND:[^ ]+]] = getelementptr inbounds nuw %struct.Pair, ptr %ref.tmp, i32 0, i32 1
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[SECOND]], ptr noundef
// CHECK: %[[FIRSTCOPY:[^ ]+]] = getelementptr inbounds nuw %struct.Pair, ptr %ref.tmp, i32 0, i32 0
// CHECK: call void @_ZN7TrackerC1ES_(ptr %t, ptr %[[FIRSTCOPY]])
// CHECK: call void @_ZN4PairD1Ev(ptr noundef nonnull align 8 dereferenceable(16) %ref.tmp)
// CHECK: call void @_ZN7TrackerD1Ev(ptr noundef nonnull align 8 dereferenceable(8) %t)

// CHECK-LABEL: define dso_local i32 @h(
extern "C" int h(int *a, int *b) {
  Tracker t = Pair{Tracker(a), Tracker(b)}.*PairFirst;
  return *a + 10 * *b;
}
// CHECK: %[[PAIRFIRST:[^ ]+]] = getelementptr inbounds nuw %struct.Pair, ptr %ref.tmp, i32 0, i32 0
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[PAIRFIRST]], ptr noundef
// CHECK: %[[PAIRSECOND:[^ ]+]] = getelementptr inbounds nuw %struct.Pair, ptr %ref.tmp, i32 0, i32 1
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[PAIRSECOND]], ptr noundef
// CHECK: %[[PAIRCOPY:[^ ]+]] = getelementptr inbounds i8, ptr %ref.tmp, i64 0
// CHECK: call void @_ZN7TrackerC1ES_(ptr %t, ptr %[[PAIRCOPY]])
// CHECK: call void @_ZN4PairD1Ev(ptr noundef nonnull align 8 dereferenceable(16) %ref.tmp)
// CHECK: call void @_ZN7TrackerD1Ev(ptr noundef nonnull align 8 dereferenceable(8) %t)

// CHECK-LABEL: define dso_local i32 @g(
extern "C" int g(int *a, int *b) {
  Tracker t = ArrayBox{Tracker(a), Tracker(b)}.elements[0];
  return *a + 10 * *b;
}
// CHECK: %[[ARRAYFIRST:[^ ]+]] = getelementptr inbounds nuw %struct.ArrayBox, ptr %ref.tmp, i32 0, i32 0
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[ARRAYFIRST]], ptr noundef
// CHECK: %[[ARRAYSECOND:[^ ]+]] = getelementptr inbounds %struct.Tracker, ptr %[[ARRAYFIRST]], i64 1
// CHECK: call void @_ZN7TrackerC1EPi(ptr noundef nonnull align 8 dereferenceable(8) %[[ARRAYSECOND]], ptr noundef
// CHECK: %[[ARRAYFIRSTCOPY:[^ ]+]] = getelementptr inbounds nuw %struct.ArrayBox, ptr %ref.tmp, i32 0, i32 0
// CHECK: %[[ARRAYIDX:[^ ]+]] = getelementptr inbounds [2 x %struct.Tracker], ptr %[[ARRAYFIRSTCOPY]], i64 0, i64 0
// CHECK: call void @_ZN7TrackerC1ES_(ptr %t, ptr %[[ARRAYIDX]])
// CHECK: call void @_ZN8ArrayBoxD1Ev(ptr noundef nonnull align 8 dereferenceable(16) %ref.tmp)
// CHECK: call void @_ZN7TrackerD1Ev(ptr noundef nonnull align 8 dereferenceable(8) %t)
