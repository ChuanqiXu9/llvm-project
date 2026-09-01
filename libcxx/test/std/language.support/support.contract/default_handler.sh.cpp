//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, windows

// RUN: %{cxx} %s %{flags} %{compile_flags} %{link_flags} -o %t.exe
// RUN: %{exec} %t.exe 2> %t.actual
// RUN: grep -Fx 'contract-handler.cpp:42:7: contract violation: precondition `value > 0` in test_function' %t.actual

#include "contract_violation_test_helper.h"

int main(int, char**) {
  contract_test::ContractViolationFixture fixture{
      std::contracts::assertion_kind::pre,
      std::contracts::evaluation_semantic::observe,
      std::contracts::detection_mode::predicate_false,
      "value > 0",
      "contract-handler.cpp",
      "test_function",
      42,
      7};

  std::contracts::invoke_default_contract_violation_handler(fixture.violation());
  return 0;
}
