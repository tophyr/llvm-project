// RUN: %clang_cc1 -std=c++26 -frelocation -fsyntax-only -verify %s

struct RelocOnly {
  RelocOnly() = default;
  RelocOnly(RelocOnly reloc);
  RelocOnly(RelocOnly&&) = delete;
  RelocOnly(const RelocOnly&) = delete;
};

struct DeletedAll {
  DeletedAll() = default;
  DeletedAll(DeletedAll&&) = delete;
  DeletedAll(const DeletedAll&) = delete;
};

struct Assignable {};

namespace std {
template <class T>
constexpr T *construct_at(T *p, T src) {
  return __builtin_construct_at_reloc(p, src.this);
}
}

void ordinary_move_assignment(Assignable &&src) {
  Assignable dst;
  dst = static_cast<Assignable &&>(src);
}

void scalar(int *p) {
  int x = __builtin_reloc_and_uninitialize(p);
  int y = __builtin_reloc_and_reclaim(p);
  (void)x;
  (void)y;
}

void record(RelocOnly *p) {
  RelocOnly x = __builtin_reloc_and_uninitialize(p);
  RelocOnly y = __builtin_reloc_and_reclaim(p);
  (void)x;
  (void)y;
}

struct ConstexprReloc {
  int value;
  constexpr ConstexprReloc(int v) : value(v) {}
  constexpr ConstexprReloc(ConstexprReloc src reloc) : value(src.value) {}
  ConstexprReloc(ConstexprReloc &&) = delete;
  ConstexprReloc(const ConstexprReloc &) = delete;
};

constexpr bool constexpr_construct_at() {
  ConstexprReloc storage(0);
  ConstexprReloc src(42);
  auto *dst = std::construct_at(&storage, reloc src);
  return dst->value == 42;
}

static_assert(constexpr_construct_at());

void bad_args(void *vp, RelocOnly v, DeletedAll *dp) {
  (void)__builtin_reloc_and_uninitialize(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  (void)__builtin_reloc_and_reclaim(v); // expected-error {{non-pointer argument to '__builtin_reloc_and_reclaim' is not allowed}}
  (void)__builtin_reloc_and_uninitialize(vp); // expected-error {{void pointer argument to '__builtin_reloc_and_uninitialize' is not allowed}}
  (void)__builtin_reloc_and_uninitialize(dp); // expected-error {{'__builtin_reloc_and_uninitialize' cannot relocate an object of type 'DeletedAll' because it has no eligible relocation, move, or copy constructor}}
}
