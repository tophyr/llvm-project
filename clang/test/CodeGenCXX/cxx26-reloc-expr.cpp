// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

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

void *address() {
  Track src reloc;
  return src.this;
}

// CHECK-LABEL: define{{.*}} @_Z7addressv(
// CHECK: [[SRC:%.*]] = alloca %struct.Track
// CHECK: ret ptr [[SRC]]
