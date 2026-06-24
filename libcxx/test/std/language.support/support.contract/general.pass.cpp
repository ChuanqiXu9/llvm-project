//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <contracts>

#include <cassert>
#include <cstring>
#include <type_traits>

#include "test_macros.h"

using ContractViolation = std::contracts::contract_violation;

static_assert(sizeof(std::contracts::assertion_kind) == 2);
static_assert(sizeof(std::contracts::evaluation_semantic) == 2);
static_assert(sizeof(std::contracts::detection_mode) == 2);

static_assert(std::is_same_v<decltype(&std::contracts::handle_contract_violation),
                             void (*)(const ContractViolation&) noexcept>);
static_assert(std::is_same_v<decltype(&std::contracts::invoke_default_contract_violation_handler),
                             void (*)(const ContractViolation&) noexcept>);

struct SourceLocationImpl {
  const char* _M_file_name;
  const char* _M_function_name;
  unsigned _M_line;
  unsigned _M_column;
};

struct ContractViolationLayout {
  __UINT16_TYPE__ _M_version;
  std::contracts::assertion_kind _M_assertion_kind;
  std::contracts::evaluation_semantic _M_evaluation_semantic;
  std::contracts::detection_mode _M_detection_mode;
  const char* _M_comment;
  const void* _M_src_loc_ptr;
  std::contracts::__vendor_ext* _M_ext;
};

int main(int, char**) {
  SourceLocationImpl loc_impl{"contracts.pass.cpp", "test_contracts", 42, 7};
  ContractViolationLayout layout{
      1,
      std::contracts::assertion_kind::post,
      std::contracts::evaluation_semantic::quick_enforce,
      std::contracts::detection_mode::evaluation_exception,
      "violation",
      &loc_impl,
      nullptr,
  };

  const auto& violation = reinterpret_cast<const ContractViolation&>(layout);
  assert(violation.kind() == std::contracts::assertion_kind::post);
  assert(violation.semantic() == std::contracts::evaluation_semantic::quick_enforce);
  assert(violation.mode() == std::contracts::detection_mode::evaluation_exception);
  assert(std::strcmp(violation.comment(), "violation") == 0);

  std::source_location location = violation.location();
  assert(std::strcmp(location.file_name(), "contracts.pass.cpp") == 0);
  assert(std::strcmp(location.function_name(), "test_contracts") == 0);
  assert(location.line() == 42u);
  assert(location.column() == 7u);
  assert(violation.is_terminating());

  layout._M_evaluation_semantic = std::contracts::evaluation_semantic::observe;
  assert(!violation.is_terminating());

  return 0;
}