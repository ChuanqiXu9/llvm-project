// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/S.cppm \
// RUN:   -emit-module-interface -o %t/S.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/A.cppm \
// RUN:   -fmodule-file=S=%t/S.pcm -emit-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/B.cppm \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -emit-module-interface -o %t/B.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/A-impl.cpp \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -emit-llvm -o /dev/null
// RUN: %clang_cc1 -std=c++20 -fcontracts -fcontract-mode=enforce %t/Use.cpp \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -fmodule-file=B=%t/B.pcm \
// RUN:   -emit-obj -o %t/Use.o
// RUN: llvm-nm --undefined-only %t/Use.o | FileCheck %s --check-prefix=NM
// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/S.cppm \
// RUN:   -emit-reduced-module-interface -o %t/S.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/A.cppm \
// RUN:   -fmodule-file=S=%t/S.pcm \
// RUN:   -emit-reduced-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/B.cppm \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -emit-reduced-module-interface -o %t/B.pcm
// RUN: %clang_cc1 -std=c++20 -fcontracts %t/A-impl.cpp \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -emit-llvm -o /dev/null
// RUN: %clang_cc1 -std=c++20 -fcontracts -fcontract-mode=enforce %t/Use.cpp \
// RUN:   -fmodule-file=S=%t/S.pcm -fmodule-file=A=%t/A.pcm \
// RUN:   -fmodule-file=B=%t/B.pcm \
// RUN:   -emit-obj -o %t/Use.o
// RUN: llvm-nm --undefined-only %t/Use.o | FileCheck %s --check-prefix=NM

//--- fancy_ptr.h
template <class T> struct fancy_ptr {
  T *value = nullptr;
};

template <class T>
inline __attribute__((visibility("hidden"),
                      exclude_from_explicit_instantiation,
                      abi_tag("nested_contract")))
bool operator==(const fancy_ptr<T> &ptr, decltype(nullptr)) {
  return ptr.value != nullptr;
}

//--- S.cppm
module;
#include "fancy_ptr.h"
export module S;

export using ::fancy_ptr;
export using ::operator==;

//--- A.cppm
export module A;

import S;

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
    const std::contracts::contract_violation &);

export template <class T>
struct Buffer {
  Buffer() post(size() == 1) {}
  unsigned size() const { return 1; }
  void trim_front(unsigned pos) noexcept pre(pos <= size()) {}
};

export template <class T>
struct ImportedNestedContractCall {
  fancy_ptr<T> ptr;
  ImportedNestedContractCall() post(!static_cast<bool>(*this)) {}
  explicit operator bool() const
      post(result : result == (ptr != nullptr)) {
    return ptr.value != nullptr;
  }
};

export bool lambda_redecl() pre([] { return true; }());

//--- A-impl.cpp
module A;

bool lambda_redecl() pre([] { return true; }()) { return true; }

//--- B.cppm
export module B;
export import A;

extern "C++" void handle_contract_violation(
    const std::contracts::contract_violation &);

export struct Holder {
  Buffer<char> buffer;
};

export inline void use_from_imported_body(Buffer<char> &buffer) {
  buffer.trim_front(0);
}

export inline Buffer<char> construct_from_imported_body() {
  return Buffer<char>();
}

export inline void assert_from_imported_generic_lambda() {
  [](auto) { contract_assert(true); }(0);
}

//--- Use.cpp
import B;

void use(Buffer<char> &buffer) {
  use_from_imported_body(buffer);
  (void)construct_from_imported_body();
  assert_from_imported_generic_lambda();
  (void)ImportedNestedContractCall<int>{};
}

// NM-NOT: nested_contract
// NM: handle_contract_violation
// NM-NOT: nested_contract
