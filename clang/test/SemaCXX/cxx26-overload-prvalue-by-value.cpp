// RUN: %clang_cc1 -std=c++23 -verify=pre26 %s
// RUN: %clang_cc1 -std=c++26 -frelocation -verify %s
// expected-no-diagnostics

struct ByConstRef {};
struct ByValue {};
struct ByRRef {};

ByConstRef scalar(const int &);
ByValue scalar(int);
ByRRef scalar(int &&);

void test_scalar() {
  int i = 0;

#if __cplusplus < 202600L
  auto ambiguous_1 = scalar(0); // pre26-error {{call to 'scalar' is ambiguous}}
                               // pre26-note@9 {{candidate function}}
                               // pre26-note@10 {{candidate function}}
                               // pre26-note@11 {{candidate function}}
  auto ambiguous_2 = scalar(static_cast<int>(i)); // pre26-error {{call to 'scalar' is ambiguous}}
                                                  // pre26-note@9 {{candidate function}}
                                                  // pre26-note@10 {{candidate function}}
                                                  // pre26-note@11 {{candidate function}}
#else
  static_assert(__is_same(decltype(scalar(0)), ByValue));
  static_assert(__is_same(decltype(scalar(static_cast<int>(i))), ByValue));
#endif
}

struct S {};

ByConstRef object(const S &);
ByValue object(S);
ByRRef object(S &&);

void test_class() {
  S s;

#if __cplusplus < 202600L
  auto ambiguous_1 = object(S{}); // pre26-error {{call to 'object' is ambiguous}}
                                  // pre26-note@33 {{candidate function}}
                                  // pre26-note@34 {{candidate function}}
                                  // pre26-note@35 {{candidate function}}
  auto ambiguous_2 = object(static_cast<S>(s)); // pre26-error {{call to 'object' is ambiguous}}
                                                // pre26-note@33 {{candidate function}}
                                                // pre26-note@34 {{candidate function}}
                                                // pre26-note@35 {{candidate function}}
#else
  static_assert(__is_same(decltype(object(S{})), ByValue));
  static_assert(__is_same(decltype(object(static_cast<S>(s))), ByValue));
#endif
}
