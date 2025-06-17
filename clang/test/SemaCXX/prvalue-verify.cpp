// RUN: %clang_cc1 -fexceptions -fcxx-exceptions -fsyntax-only -verify %s

struct PrvalueOnly {
  constexpr PrvalueOnly(int val)
    : val_{val} {}
  
  PrvalueOnly(PrvalueOnly&&~) = delete;
  PrvalueOnly& operator=(PrvalueOnly&&~) = delete;
  
  PrvalueOnly(PrvalueOnly&&) = delete;
  PrvalueOnly& operator=(PrvalueOnly&&) = delete;
  PrvalueOnly(const PrvalueOnly&) = delete;
  PrvalueOnly& operator=(const PrvalueOnly&) = delete;
  
  int val_;
};