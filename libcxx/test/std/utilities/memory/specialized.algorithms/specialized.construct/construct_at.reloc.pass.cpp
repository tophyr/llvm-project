// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <cassert>
#include <memory>
#include <new>

struct RelocOnly {
  int* relocations;
  int* destructions;

  constexpr RelocOnly(int& r, int& d) : relocations(&r), destructions(&d) {}
  constexpr RelocOnly(RelocOnly src reloc)
      : relocations(src.relocations), destructions(src.destructions) {
    ++*relocations;
  }
  RelocOnly(RelocOnly&&) = delete;
  RelocOnly(const RelocOnly&) = delete;
  constexpr ~RelocOnly() { ++*destructions; }
};

int main(int, char**) {
  int relocations = 0;
  int destructions = 0;
  alignas(RelocOnly) unsigned char storage[sizeof(RelocOnly)];

  RelocOnly src(relocations, destructions);
  RelocOnly* dst = std::construct_at(reinterpret_cast<RelocOnly*>(storage), reloc src);
  assert(dst != nullptr);
  assert(relocations == 1);
  assert(destructions == 0);

  std::destroy_at(dst);
  assert(destructions == 1);
  return 0;
}
