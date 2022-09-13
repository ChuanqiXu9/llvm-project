// RUN: rm -fr %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++20 -E %t/M.cppm -fdep-format=trtbd -fdep-output=%t/M.cppm.o -fdep-file=%t/M.dep.json &> /dev/null
// RUN: cat %t/M.dep.json | FileCheck %t/M.cppm
// RUN: %clang_cc1 -std=c++20 -E %t/Impl.cpp -fdep-format=trtbd -fdep-output=%t/Impl.cpp.o -fdep-file=%t/Impl.dep.json &> /dev/null
// RUN: cat %t/Impl.dep.json | FileCheck %t/Impl.cpp
// RUN: %clang_cc1 -std=c++20 -E %t/impl_part.cppm -fdep-format=trtbd -fdep-output=%t/M-impl_part.cppm.o -fdep-file=%t/impl_part.dep.json &> /dev/null
// RUN: cat %t/impl_part.dep.json | FileCheck %t/impl_part.cppm
// RUN: %clang_cc1 -std=c++20 -E %t/interface_part.cppm -fdep-format=trtbd -fdep-output=%t/M-interface_part.cppm.o -fdep-file=%t/interface_part.dep.json &> /dev/null
// RUN: cat %t/interface_part.dep.json | FileCheck %t/interface_part.cppm
// RUN: %clang_cc1 -std=c++20 -E %t/User.cpp -fdep-format=trtbd -fdep-output=%t/User.cpp.o -fdep-file=%t/User.dep.json &> /dev/null
// RUN: cat %t/User.dep.json | FileCheck %t/User.cpp

//--- M.cppm
export module M;
export import :interface_part;
import :impl_part;
export void Hello();

// CHECK: {
// CHECK:   "version": 1,
// CHECK:   "revision": 0,
// CHECK:   "rules": [
// CHECK:     {
// CHECK:       "primary-output": "{{.*}}M.cppm.o",
// CHECK:       "provides": [
// CHECK:         {
// CHECK:           "logical-name": "M"
// CHECK:         }
// CHECK:       ],
// CHECK:       "requires": [
// CHECK:         {
// CHECK:           "logical-name": "M:interface_part"
// CHECK:         },
// CHECK:         {
// CHECK:           "logical-name": "M:impl_part"
// CHECK:         }
// CHECK:       ]
// CHECK:     }
// CHECK:   ]
// CHECK: }

//--- Impl.cpp
module;
#include "header.mock"
module M;
void Hello() {
    std::cout << "Hello ";
}

// CHECK: {
// CHECK:   "version": 1,
// CHECK:   "revision": 0,
// CHECK:   "rules": [
// CHECK:     {
// CHECK:       "primary-output": "{{.*}}Impl.cpp.o",
// CHECK:       "requires": [
// CHECK:         {
// CHECK:           "logical-name": "M"
// CHECK:         }
// CHECK:       ]
// CHECK:     }
// CHECK:   ]
// CHECK: }


//--- impl_part.cppm
module;
#include "header.mock"
module M:impl_part;
import :interface_part;

std::string W = "World.";
void World() {
    std::cout << W << std::endl;
}

// CHECK: {
// CHECK:   "version": 1,
// CHECK:   "revision": 0,
// CHECK:   "rules": [
// CHECK:     {
// CHECK:       "primary-output": "{{.*}}M-impl_part.cppm.o",
// CHECK:       "provides": [
// CHECK:         {
// CHECK:           "logical-name": "M:impl_part",
// CHECK:           "is-interface": false
// CHECK:         }
// CHECK:       ],
// CHECK:       "requires": [
// CHECK:         {
// CHECK:           "logical-name": "M:interface_part"
// CHECK:         }
// CHECK:       ]
// CHECK:     }
// CHECK:   ]
// CHECK: }


//--- interface_part.cppm
export module M:interface_part;
export void World();

// CHECK: {
// CHECK:   "version": 1,
// CHECK:   "revision": 0,
// CHECK:   "rules": [
// CHECK:     {
// CHECK:       "primary-output": "{{.*}}M-interface_part.cppm.o",
// CHECK:       "provides": [
// CHECK:         {
// CHECK:           "logical-name": "M:interface_part"
// CHECK:         }
// CHECK:       ]
// CHECK:     }
// CHECK:   ]
// CHECK: }

//--- User.cpp
import M;
int main() {
    Hello();
    World();
    return 0;
}

// CHECK: {
// CHECK:   "version": 1,
// CHECK:   "revision": 0,
// CHECK:   "rules": [
// CHECK:     {
// CHECK:       "primary-output": "{{.*}}User.cpp.o",
// CHECK:       "requires": [
// CHECK:         {
// CHECK:           "logical-name": "M"
// CHECK:         }
// CHECK:       ]
// CHECK:     }
// CHECK:   ]
// CHECK: }

//--- header.mock
