// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct R {
  int value;
  constexpr R(int value = 0) : value(value) {}
  constexpr R(const R &) = delete;
  constexpr R(R &&) = delete; // expected-note 2 {{'R' has been explicitly marked deleted here}}
  constexpr R(R reloc) = default;
};

struct W {
  R r;
};

constexpr W makeW() { return {{1}}; }
constexpr int test_member_init() {
  R x = makeW().r;
  return x.value;
}
static_assert(test_member_init() == 1);

struct WithArray {
  R elements[3];
};

constexpr WithArray makeWithArray() { return {{{1}, {2}, {3}}}; }
constexpr int test_array_init() {
  R x = makeWithArray().elements[1];
  return x.value;
}
static_assert(test_array_init() == 2);

void sink(R);
void test_member_arg() { sink(makeW().r); }

struct B {
  int value;
  constexpr B(int value = 0) : value(value) {}
  constexpr B(const B &) = delete;
  constexpr B(B &&) = delete;
  constexpr B(B reloc) = default;
};

struct D : B {
  constexpr D(int value = 0) : B(value) {}
};

constexpr int test_base_init() {
  B b = static_cast<B &&>(D{2});
  return b.value;
}
static_assert(test_base_init() == 2);

constexpr R W::*MemberPtr = &W::r;
constexpr int test_member_pointer_init() {
  R x = makeW().*MemberPtr;
  return x.value;
}
static_assert(test_member_pointer_init() == 1);

struct BR {
  R r;
};

struct DR : BR {};

constexpr DR makeDR() { return {{{4}}}; }
constexpr R BR::*BaseMemberPtr = &BR::r;
constexpr int test_base_member_pointer_init() {
  R x = makeDR().*BaseMemberPtr;
  return x.value;
}
static_assert(test_base_member_pointer_init() == 4);

struct WithPrivate {
  R pub;

private:
  R hidden;
};

WithPrivate makeWithPrivate();
void test_private_fallback() {
  R x = makeWithPrivate().pub; // expected-error {{call to deleted constructor of 'R'}}
}

struct WithUserProvidedDtor {
  R r;
  ~WithUserProvidedDtor() {}
};

WithUserProvidedDtor makeWithUserProvidedDtor();
void test_user_provided_dtor_fallback() {
  R x = makeWithUserProvidedDtor().r; // expected-error {{call to deleted constructor of 'R'}}
}
