// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/Std.cppm \
// RUN:   -emit-module-interface -o %t/Std.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/API.cppm \
// RUN:   -fmodule-file=Std=%t/Std.pcm -emit-module-interface -o %t/API.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/Use.cppm \
// RUN:   -fmodule-file=Std=%t/Std.pcm -fmodule-file=API=%t/API.pcm \
// RUN:   -fsyntax-only -verify

//--- Std.cppm
export module Std;

export namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};

namespace contracts {
enum class assertion_kind : unsigned short { pre = 1, post = 2, assert = 3 };
enum class evaluation_semantic : unsigned short {
  ignore = 1,
  observe = 2,
  enforce = 3,
  quick_enforce = 4
};
enum class detection_mode : unsigned short {
  predicate_false = 1,
  evaluation_exception = 2
};

class contract_violation {
  unsigned short _M_version;
  assertion_kind _M_assertion_kind;
  evaluation_semantic _M_evaluation_semantic;
  detection_mode _M_detection_mode;
  const char *_M_comment;
  const void *_M_src_loc_ptr;
  void *_M_ext;
};
} // namespace contracts
} // namespace std

extern "C++" void handle_contract_violation(
    const std::contracts::contract_violation &) noexcept;

//--- API.cppm
export module API;
import Std;

extern "C++" void handle_contract_violation(
    const std::contracts::contract_violation &) noexcept;

export struct Result {
  int size() const { return 1; }

  template <class Index>
  int operator[](Index index) const
    pre(index >= 0) pre(index < size()) {
    return index;
  }

  template <class F>
  static Result build(F &&fill) post(result: result.size() == 1) {
    fill();
    return {};
  }
};

export template <class T>
struct LateMemberTemplateContract {
  LateMemberTemplateContract() post(this->size() == 0) {}

  int size() const { return 0; }
};

//--- Use.cppm
export module Use;
import Std;
import API;

extern "C++" void handle_contract_violation(
    const std::contracts::contract_violation &) noexcept;

// expected-no-diagnostics
LateMemberTemplateContract<int> late_member_template_contract;

template <bool Value, class F>
decltype(auto) dispatch(F &&f) {
  return static_cast<F &&>(f).template operator()<Value>();
}

template <class F>
void use_nested_generic_lambdas(Result value, F &&f) {
  dispatch<true>([&]<bool A> {
    dispatch<true>([&]<bool B> {
      dispatch<true>([&]<bool C> {
        f(value[0]);
      });
    });
  });
}
