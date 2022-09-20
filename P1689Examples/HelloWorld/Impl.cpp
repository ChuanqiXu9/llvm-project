// clang++ -fdep-format=trtbd -fdep-output=Impl.cpp.o -fdep-file=Impl.dep.json Impl.cpp -std=c++20 -E
module;
#include <iostream>
module Hello;
void Hello() {
    std::cout << "Hello ";
}
