// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <cassert>
#include <memory>
#include <new>

struct HeapRelocOnly {
  static int relocations;
  static int destructions;
  static int deletes;

  HeapRelocOnly() = default;
  HeapRelocOnly(HeapRelocOnly reloc) { ++relocations; }
  HeapRelocOnly(HeapRelocOnly&&) = delete;
  HeapRelocOnly(const HeapRelocOnly&) = delete;
  ~HeapRelocOnly() { ++destructions; }

  static void operator delete(void* p) {
    ++deletes;
    ::operator delete(p);
  }
};

int HeapRelocOnly::relocations = 0;
int HeapRelocOnly::destructions = 0;
int HeapRelocOnly::deletes = 0;

struct MoveFallback {
  static int destructions;

  MoveFallback() = default;
  MoveFallback(MoveFallback&&) {}
  MoveFallback(const MoveFallback&) = delete;
  ~MoveFallback() { ++destructions; }
};

int MoveFallback::destructions = 0;

int main(int, char**) {
  alignas(MoveFallback) unsigned char storage[sizeof(MoveFallback)];
  auto* placed = ::new (static_cast<void*>(storage)) MoveFallback();
  MoveFallback moved = std::reloc_and_uninitialize(placed);
  (void)moved;
  assert(MoveFallback::destructions == 1);

  auto* heap = new HeapRelocOnly();
  HeapRelocOnly extracted = std::reloc_and_reclaim(heap);
  (void)extracted;
  assert(HeapRelocOnly::relocations == 1);
  assert(HeapRelocOnly::destructions == 0);
  assert(HeapRelocOnly::deletes == 1);

  return 0;
}
