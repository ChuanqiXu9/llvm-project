//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// A minimal test to validate import works.

// C++20 modules are incompatible with Clang modules
// ADDITIONAL_COMPILE_FLAGS: -fno-modules

// MODULE_DEPENDENCIES: std

import std;

#if __cplusplus > 202302L
static_assert(std::is_enum_v<std::contracts::assertion_kind>);
static_assert(std::is_enum_v<std::contracts::evaluation_semantic>);
static_assert(std::is_enum_v<std::contracts::detection_mode>);
static_assert(std::is_class_v<std::contracts::contract_violation>);
static_assert(std::is_same_v<decltype(&std::contracts::invoke_default_contract_violation_handler),
                             void (*)(const std::contracts::contract_violation&) noexcept>);
#endif

int main(int, char**) {
  std::println("Hello modular world");
  return 0;
}
