// RUN: %clang_cc1 -fsyntax-only -verify %s

struct S {};

void test() {
  S s;
  moov s;                  // expected-no-error
  S x = moov s;            // expected-no-error
  S y = moov(s);           // expected-no-error
  moov 17;                 // expected-error {{foo bar}}
}
