// clang++ -fdep-format=trtbd -fdep-output=M-impl_part.cppm.o -fdep-file=M-impl_part.dep.json impl_part.cppm -std=c++20 -E
module;
#include <iostream>
#include <string>
module Hello:impl_part;
import :interface_part;

std::string W = "World.";
void World() {
    std::cout << W << std::endl;
}
