// RUN: rm -rf %t && split-file %s %t
// RUN: %clang_cc1 -std=c++2c -fcontracts %t/M.cppm \
// RUN:   -emit-module-interface -o %t/M.pcm
// RUN: not %clang_cc1 -std=c++2c %t/Use.cpp \
// RUN:   -fmodule-file=M=%t/M.pcm -fsyntax-only 2>&1 | \
// RUN:   FileCheck %s --check-prefix=CONTRACTS
// RUN: not %clang_cc1 -std=c++2c -fcontracts -fcontract-mode=observe \
// RUN:   %t/Use.cpp -fmodule-file=M=%t/M.pcm -fsyntax-only 2>&1 | \
// RUN:   FileCheck %s --check-prefix=MODE

// CONTRACTS: error: C++ contracts support was enabled in precompiled file '{{.*}}M.pcm' but is currently disabled
// MODE: error: C++ contract violation handling mode differs in precompiled file '{{.*}}M.pcm' vs. current file

//--- M.cppm
export module M;
export int value();

//--- Use.cpp
import M;
