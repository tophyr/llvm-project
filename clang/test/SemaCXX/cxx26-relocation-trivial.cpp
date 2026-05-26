// RUN: %clang_cc1 -std=c++26 -frelocation -ast-dump %s | FileCheck %s

struct TrivialCopyFallbackMember {
  int value;
  TrivialCopyFallbackMember() = default;
  TrivialCopyFallbackMember(const TrivialCopyFallbackMember &) = default;
  TrivialCopyFallbackMember(TrivialCopyFallbackMember &&) = delete;
  TrivialCopyFallbackMember &
  operator=(const TrivialCopyFallbackMember &) = default;
  TrivialCopyFallbackMember &
  operator=(TrivialCopyFallbackMember &&) = delete;
};

struct TrivialDefaultedCopyFallback {
  TrivialCopyFallbackMember member;
  TrivialDefaultedCopyFallback(TrivialDefaultedCopyFallback reloc) = default;
};

struct TrivialDefaultedCopyAssignFallback {
  TrivialCopyFallbackMember member;
  TrivialDefaultedCopyAssignFallback &
  operator=(TrivialDefaultedCopyAssignFallback src reloc) = default;
};

// CHECK: CXXConstructorDecl {{.*}} TrivialDefaultedCopyFallback 'void (TrivialDefaultedCopyFallback)' default trivial
// CHECK: CXXMethodDecl {{.*}} operator= 'TrivialDefaultedCopyAssignFallback &(TrivialDefaultedCopyAssignFallback)' default trivial
