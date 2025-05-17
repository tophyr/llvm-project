// RUN: %clang_cc1 -fsyntax-only -verify %s

int x = moov;        // expected-error {{expected expression after 'moov'}}
int y = moov 42 17;  // expected-error {{expected ';'}}
