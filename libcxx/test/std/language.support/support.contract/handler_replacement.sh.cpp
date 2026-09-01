//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, windows

// ADDITIONAL_COMPILE_FLAGS: -fcontracts -fcontract-mode=observe

// Make the replacement and its caller separate translation units so the test
// exercises symbol replacement rather than a direct call within one unit.
// Calling the default handler ensures the runtime definition is linked too;
// with a static libc++ this forces the object containing the weak handler into
// the link.
// RUN: %{cxx} %s %{flags} %{compile_flags} -DTEST_HANDLER -c -o %t.handler.o
// RUN: %{cxx} %s %{flags} %{compile_flags} -DTEST_MAIN -c -o %t.main.o
// RUN: %{cxx} %t.handler.o %t.main.o %{flags} %{link_flags} -o %t.exe
// RUN: %{exec} %t.exe 2> %t.actual

#include "contract_violation_test_helper.h"

#include <cassert>
#include <cstring>

#if defined(TEST_HANDLER)

int handler_calls = 0;

void handle_contract_violation(const std::contracts::contract_violation& violation) {
  ++::handler_calls;
  assert(violation.kind() == std::contracts::assertion_kind::post);
  assert(violation.semantic() == std::contracts::evaluation_semantic::observe);
  assert(violation.detection_mode() == std::contracts::detection_mode::predicate_false);
  assert(std::strcmp(violation.comment(), "result != 0") == 0);
}

#elif defined(TEST_MAIN)

extern int handler_calls;

int checked_value(int value) post(result : result != 0) { return value; }

int main(int, char**) {
  contract_test::ContractViolationFixture fixture{
      std::contracts::assertion_kind::post,
      std::contracts::evaluation_semantic::observe,
      std::contracts::detection_mode::predicate_false,
      "result != 0",
      "contract-handler.cpp",
      "test_function",
      42,
      7};

  checked_value(0);
  assert(handler_calls == 1);
  std::contracts::invoke_default_contract_violation_handler(fixture.violation());
  assert(handler_calls == 1);
  return 0;
}

#endif
