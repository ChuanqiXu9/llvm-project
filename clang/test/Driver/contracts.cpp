// RUN: %clang -std=c++2c -fcontracts -### %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -std=c++2c -fcontracts -fno-contracts -### %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang -std=c++2c -fcontracts -fcontract-mode=observe -### %s 2>&1 | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang -std=c++2c -fcontracts -fcontract-mode=ignore -### %s 2>&1 | FileCheck %s --check-prefix=IGNORE

// ON: "-fcontracts"
// ON-NOT: "-fcontract-mode=

// OFF-NOT: "-fcontracts"

// OBSERVE: "-fcontracts"
// OBSERVE: "-fcontract-mode=observe"

// IGNORE: "-fcontracts"
// IGNORE: "-fcontract-mode=ignore"
