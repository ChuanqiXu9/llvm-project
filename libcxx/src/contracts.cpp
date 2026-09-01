//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// libc++ is currently built in C++23 mode, while the Contracts library types
// are a C++26 facility. Make them available while building their runtime.
#define __cpp_contracts 202502L

#include <contracts>
#include <cstdio>

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

namespace contracts {

void invoke_default_contract_violation_handler(const contract_violation& violation) noexcept {
  const source_location location = violation.location();
  const char* kind               = "contract assertion";
  switch (violation.kind()) {
  case assertion_kind::pre:
    kind = "precondition";
    break;
  case assertion_kind::post:
    kind = "postcondition";
    break;
  case assertion_kind::assert:
    break;
  }

  std::fprintf(
      stderr,
      "%s:%u:%u: contract violation: %s `%s` in %s\n",
      location.file_name(),
      location.line(),
      location.column(),
      kind,
      violation.comment(),
      location.function_name());
}

} // namespace contracts

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD

_LIBCPP_EXPORTED_FROM_ABI __attribute__((__weak__)) void
handle_contract_violation(const std::contracts::contract_violation&) noexcept;

_LIBCPP_EXPORTED_FROM_ABI __attribute__((__weak__)) void
handle_contract_violation(const std::contracts::contract_violation& violation) noexcept {
  std::contracts::invoke_default_contract_violation_handler(violation);
}
