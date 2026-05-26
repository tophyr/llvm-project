// RUN: %clang_cc1 -std=c++26 -fsyntax-only %s

struct CopyOnlyNoexcept {
  CopyOnlyNoexcept() = default;
  CopyOnlyNoexcept(const CopyOnlyNoexcept &) noexcept = default;
  CopyOnlyNoexcept(CopyOnlyNoexcept &&) = delete;
  CopyOnlyNoexcept &operator=(const CopyOnlyNoexcept &) noexcept = default;
  CopyOnlyNoexcept &operator=(CopyOnlyNoexcept &&) = delete;
};

struct WrapCtor {
  CopyOnlyNoexcept value;
  WrapCtor(WrapCtor reloc) = default;
};

struct WrapAssign {
  CopyOnlyNoexcept value;
  WrapAssign &operator=(WrapAssign src reloc) = default;
};

static_assert(noexcept(WrapCtor(*static_cast<WrapCtor *>(nullptr))));
static_assert(noexcept(
    (*static_cast<WrapAssign *>(nullptr))
        .operator=(WrapAssign(*static_cast<WrapAssign *>(nullptr)))));
