// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

template <typename T>
T move_like(T t) {
  return moov t; // expected-no-error
}

void test() {
  int x = 5;
  int y = move_like(x); // expected-no-error
}
