// RUN: %clang_cc1 -std=c++26 -frelocation -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

struct Cleanup {
  ~Cleanup();
};

struct Track {
  int *ptr;
  Cleanup cleanup;

  Track();
  Track(const Track &) = delete;
  Track(Track &&) = delete;
  Track(Track src reloc) : ptr(src.ptr) {}
  ~Track() = default;
};

namespace std {
template <class T>
constexpr T *construct_at(T *p, T src) {
  return __builtin_construct_at_reloc(p, src.this);
}
}

void by_value_arg(Track src) {}

void local() {
  Track src;
  Track dst = reloc src;
}

// CHECK-LABEL: define{{.*}} @_Z5localv(
// CHECK: call void @_ZN5TrackD1Ev(
// CHECK-NOT: call void @_ZN5TrackD1Ev(
// CHECK: ret void

void call_by_value(Track src) {
  by_value_arg(reloc src);
}

// CHECK-LABEL: define{{.*}} @_Z13call_by_value5Track(
// CHECK: call void @_ZN5TrackC1ES_
// CHECK-NOT: call void @_ZN5TrackC1ES_
// CHECK: call void @_Z12by_value_arg5Track
// CHECK: ret void

void call_construct_at() {
  Track src;
  alignas(Track) unsigned char storage[sizeof(Track)];
  (void)std::construct_at(reinterpret_cast<Track *>(storage), reloc src);
}

// CHECK-LABEL: define{{.*}} @_Z17call_construct_atv(
// CHECK: call void @_ZN5TrackC1ES_
// CHECK-NOT: call void @_ZN5TrackC1ES_
// CHECK: ret void

void param(Track src) {
  Track dst = reloc src;
}

// CHECK-LABEL: define{{.*}} @_Z5param5Track(
// CHECK: call void @_ZN5TrackD1Ev(
// CHECK-NOT: call void @_ZN5TrackD1Ev(
// CHECK: ret void

void *address() {
  Track src reloc;
  return src.this;
}

// CHECK-LABEL: define{{.*}} @_Z7addressv(
// CHECK: [[SRC:%.*]] = alloca %struct.Track
// CHECK: ret ptr [[SRC]]
