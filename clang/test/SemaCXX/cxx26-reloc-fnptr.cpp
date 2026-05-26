// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

struct S {
  int value;
};

void free_ok(S arg);

void (*function_pointer_bad)(S reloc); // expected-error {{'reloc' parameter is only permitted in a function declaration}}
void (&function_reference_bad)(S reloc) = free_ok; // expected-error {{'reloc' parameter is only permitted in a function declaration}}
