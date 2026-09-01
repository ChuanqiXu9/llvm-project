//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_LANGUAGE_SUPPORT_SUPPORT_CONTRACT_CONTRACT_VIOLATION_TEST_HELPER_H
#define TEST_STD_LANGUAGE_SUPPORT_SUPPORT_CONTRACT_CONTRACT_VIOLATION_TEST_HELPER_H

#include <contracts>

namespace contract_test {

struct SourceLocationLayout {
  const char* file_name;
  const char* function_name;
  unsigned line;
  unsigned column;
};

struct ContractViolationLayout {
  __UINT16_TYPE__ version;
  std::contracts::assertion_kind kind;
  std::contracts::evaluation_semantic semantic;
  std::contracts::detection_mode mode;
  const char* comment;
  const void* source_location;
  std::contracts::__vendor_ext* extension;
};

struct ContractViolationFixture {
  SourceLocationLayout source_location;
  ContractViolationLayout layout;

  ContractViolationFixture(
      std::contracts::assertion_kind kind,
      std::contracts::evaluation_semantic semantic,
      std::contracts::detection_mode mode,
      const char* comment,
      const char* file_name,
      const char* function_name,
      unsigned line,
      unsigned column)
      : source_location{file_name, function_name, line, column},
        layout{1, kind, semantic, mode, comment, &source_location, nullptr} {}

  const std::contracts::contract_violation& violation() const {
    return reinterpret_cast<const std::contracts::contract_violation&>(layout);
  }
};

} // namespace contract_test

#endif // TEST_STD_LANGUAGE_SUPPORT_SUPPORT_CONTRACT_CONTRACT_VIOLATION_TEST_HELPER_H
