//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, windows
// REQUIRES: clang

// ADDITIONAL_COMPILE_FLAGS: -fcontracts -fcontract-mode=observe

// RUN: %{build}
// RUN: %{exec} %t.exe 2> %t.actual
// RUN: grep -F 'contract violation: precondition `value > 0`' %t.actual

#include <contracts>

int checked_value(int value) pre(value > 0) { return value; }

int main(int, char**) {
  checked_value(0);
  return 0;
}
